/*
 * Intermediate code analyzation
 * Copyright (C) 2014  Antonie Blom
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef ITM_ANALYZE_H
#define ITM_ANALYZE_H

#include <acc/itm/ast.h>
#include <acc/itm/tag.h>

/*
 * The tags used to represent analysis information provided by analyze()
 */
extern const tagtype_t tt_used;
extern const tagtype_t tt_endlife;
extern const tagtype_t tt_phiable;

/*
 * Analysis flags to pass to analyze()
 */
enum analysis {
	/*
	 * Analyzes whether SSA-nodes are used
	 */
	A_USED = 0x1,
	/*
	 * Tags the lifetime of SSA-nodes
	 */
	A_LIFETIME = 0x2,
	/*
	 * Tags 'phiable' nodes as such
	 */
	A_PHIABLE = 0x4
};

void analyze(struct itm_block *strt, enum analysis a);

#endif
