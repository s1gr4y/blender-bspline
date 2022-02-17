/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

/** \file
 * \ingroup spassets
 */

#pragma once

void asset_browser_operatortypes();

struct ARegion;
struct ARegionType;
struct AssetLibrary;
struct bContext;
struct PointerRNA;
struct PropertyRNA;
struct uiLayout;
struct wmMsgBus;

void asset_browser_main_region_draw(const bContext *C, ARegion *region);

void asset_browser_navigation_region_panels_register(ARegionType *art);

void asset_view_create_catalog_tree_view_in_layout(::AssetLibrary *asset_library,
                                                   uiLayout *layout,
                                                   PointerRNA *catalog_filter_owner_ptr,
                                                   PropertyRNA *catalog_filter_prop,
                                                   wmMsgBus *msg_bus);