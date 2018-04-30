/* $Id: industry_sl.cpp 26175 2013-12-23 18:09:29Z frosch $ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file industry_sl.cpp Code handling saving and loading of industries */

#include "../stdafx.h"
#include "../industry.h"

#include "saveload.h"
#include "newgrf_sl.h"

#include "../safeguards.h"

static OldPersistentStorage _old_ind_persistent_storage;

static const SaveLoad _industry_desc[] = {
	SLE_CONDVAR(Industry, location.tile,              SLE_FILE_U16 | SLE_VAR_U32,  0, 5),
	SLE_CONDVAR(Industry, location.tile,              SLE_UINT32,                  6, SL_MAX_VERSION),
	    SLE_VAR(Industry, location.w,                 SLE_FILE_U8 | SLE_VAR_U16),
	    SLE_VAR(Industry, location.h,                 SLE_FILE_U8 | SLE_VAR_U16),
	    SLE_REF(Industry, town,                       REF_TOWN),
	SLE_CONDNULL( 2, 0, 60),       ///< used to be industry's produced_cargo
	SLE_CONDARR(Industry, produced_cargo,             SLE_UINT8,  2,              78, SL_MAX_VERSION),
	SLE_CONDARR(Industry, incoming_cargo_waiting,     SLE_UINT16, 3,              70, SL_MAX_VERSION),
	    SLE_ARR(Industry, produced_cargo_waiting,     SLE_UINT16, 2),
	    SLE_ARR(Industry, production_rate,            SLE_UINT8,  2),
	SLE_CONDNULL( 3, 0, 60),       ///< used to be industry's accepts_cargo
	SLE_CONDARR(Industry, accepts_cargo,              SLE_UINT8,  3,              78, SL_MAX_VERSION),
	    SLE_VAR(Industry, prod_level,                 SLE_UINT8),
	    SLE_ARR(Industry, this_month_production,      SLE_UINT16, 2),
	    SLE_ARR(Industry, this_month_transported,     SLE_UINT16, 2),
	    SLE_ARR(Industry, last_month_pct_transported, SLE_UINT8,  2),
	    SLE_ARR(Industry, last_month_production,      SLE_UINT16, 2),
	    SLE_ARR(Industry, last_month_transported,     SLE_UINT16, 2),

	    SLE_VAR(Industry, counter,                    SLE_UINT16),

	    SLE_VAR(Industry, type,                       SLE_UINT8),
	    SLE_VAR(Industry, owner,                      SLE_UINT8),
	    SLE_VAR(Industry, random_colour,              SLE_UINT8),
	SLE_CONDVAR(Industry, last_prod_year,             SLE_FILE_U8 | SLE_VAR_I32,  0, 30),
	SLE_CONDVAR(Industry, last_prod_year,             SLE_INT32,                 31, SL_MAX_VERSION),
	    SLE_VAR(Industry, was_cargo_delivered,        SLE_UINT8),

	SLE_CONDVAR(Industry, founder,                    SLE_UINT8,                 70, SL_MAX_VERSION),
	SLE_CONDVAR(Industry, construction_date,          SLE_INT32,                 70, SL_MAX_VERSION),
	SLE_CONDVAR(Industry, construction_type,          SLE_UINT8,                 70, SL_MAX_VERSION),
	SLE_CONDVAR(Industry, last_cargo_accepted_at,     SLE_INT32,                 70, SL_MAX_VERSION),
	SLE_CONDVAR(Industry, selected_layout,            SLE_UINT8,                 73, SL_MAX_VERSION),

	SLEG_CONDARR(_old_ind_persistent_storage.storage, SLE_UINT32, 16,            76, 160),
	SLE_CONDREF(Industry, psa,                        REF_STORAGE,              161, SL_MAX_VERSION),

	SLE_CONDNULL(1, 82, 196),                             // random_triggers
	SLE_CONDNULL(1, SL_PATCH_PACK, SL_PATCH_PACK_1_18_6), // random_triggers

	SLE_CONDVAR(Industry, random,                     SLE_UINT16,                82, SL_MAX_VERSION),

	SLE_CONDNULL(32, 2, 143), // old reserved space

	SLE_END()
};

static void Save_INDY()
{
	Industry *ind;

	/* Write the industries */
	FOR_ALL_INDUSTRIES(ind) {
		SlSetArrayIndex(ind->index);
		SlObject(ind, _industry_desc);
	}
}

static void Save_IIDS()
{
	Save_NewGRFMapping(_industry_mngr);
}

static void Save_TIDS()
{
	Save_NewGRFMapping(_industile_mngr);
}

static void Load_INDY()
{
	int index;

	Industry::ResetIndustryCounts();

	while ((index = SlIterateArray()) != -1) {
		Industry *i = new (index) Industry();
		SlObject(i, _industry_desc);

		/* Before savegame version 161, persistent storages were not stored in a pool. */
		if (IsSavegameVersionBefore(161) && !IsSavegameVersionBefore(76)) {
			/* Store the old persistent storage. The GRFID will be added later. */
			assert(PersistentStorage::CanAllocateItem());
			i->psa = new PersistentStorage(0, 0, 0);
			memcpy(i->psa->storage, _old_ind_persistent_storage.storage, sizeof(i->psa->storage));
		}
		Industry::IncIndustryTypeCount(i->type);
	}
}

static void Load_IIDS()
{
	Load_NewGRFMapping(_industry_mngr);
}

static void Load_TIDS()
{
	Load_NewGRFMapping(_industile_mngr);
}

static void Ptrs_INDY()
{
	Industry *i;

	FOR_ALL_INDUSTRIES(i) {
		SlObject(i, _industry_desc);
	}
}

/** Description of the data to save and load in #IndustryBuildData. */
static const SaveLoad _industry_builder_desc[] = {
	SLEG_VAR(_industry_builder.wanted_inds, SLE_UINT32),
	SLEG_END()
};

/** Load/save industry builder. */
static void LoadSave_IBLD()
{
	SlGlobList(_industry_builder_desc);
}

/** Description of the data to save and load in #IndustryTypeBuildData. */
static const SaveLoad _industrytype_builder_desc[] = {
	SLE_VAR(IndustryTypeBuildData, probability,  SLE_UINT32),
	SLE_VAR(IndustryTypeBuildData, min_number,   SLE_UINT8),
	SLE_VAR(IndustryTypeBuildData, target_count, SLE_UINT16),
	SLE_VAR(IndustryTypeBuildData, max_wait,     SLE_UINT16),
	SLE_VAR(IndustryTypeBuildData, wait_count,   SLE_UINT16),
	SLE_END()
};

/** Save industry-type build data. */
static void Save_ITBL()
{
	for (int i = 0; i < NUM_INDUSTRYTYPES; i++) {
		SlSetArrayIndex(i);
		SlObject(_industry_builder.builddata + i, _industrytype_builder_desc);
	}
}

/** Load industry-type build data. */
static void Load_ITBL()
{
	for (IndustryType it = 0; it < NUM_INDUSTRYTYPES; it++) {
		_industry_builder.builddata[it].Reset();
	}
	int index;
	while ((index = SlIterateArray()) != -1) {
		if ((uint)index >= NUM_INDUSTRYTYPES) SlErrorCorrupt("Too many industry builder datas");
		SlObject(_industry_builder.builddata + index, _industrytype_builder_desc);
	}
}

extern const ChunkHandler _industry_chunk_handlers[] = {
	{ 'INDY', Save_INDY,     Load_INDY,     Ptrs_INDY, nullptr, CH_ARRAY},
	{ 'IIDS', Save_IIDS,     Load_IIDS,     nullptr,      nullptr, CH_ARRAY},
	{ 'TIDS', Save_TIDS,     Load_TIDS,     nullptr,      nullptr, CH_ARRAY},
	{ 'IBLD', LoadSave_IBLD, LoadSave_IBLD, nullptr,      nullptr, CH_RIFF},
	{ 'ITBL', Save_ITBL,     Load_ITBL,     nullptr,      nullptr, CH_ARRAY | CH_LAST},
};
