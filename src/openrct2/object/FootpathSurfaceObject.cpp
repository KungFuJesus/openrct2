/*****************************************************************************
 * Copyright (c) 2014-2022 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "FootpathSurfaceObject.h"

#include "../core/IStream.hpp"
#include "../core/Json.hpp"
#include "../drawing/Image.h"
#include "../object/ObjectRepository.h"

void FootpathSurfaceObject::Load()
{
    GetStringTable().Sort();
    NameStringId = language_allocate_object_string(GetName());

    auto numImages = GetImageTable().GetCount();
    if (numImages != 0)
    {
        PreviewImageId = gfx_object_allocate_images(GetImageTable().GetImages(), GetImageTable().GetCount());
        BaseImageId = PreviewImageId + 1;
    }

    _descriptor.Name = NameStringId;
    _descriptor.Image = BaseImageId;
    _descriptor.PreviewImage = PreviewImageId;
    _descriptor.Flags = Flags;
}

void FootpathSurfaceObject::Unload()
{
    language_free_object_string(NameStringId);
    gfx_object_free_images(PreviewImageId, GetImageTable().GetCount());

    NameStringId = 0;
    PreviewImageId = 0;
    BaseImageId = 0;
}

void FootpathSurfaceObject::DrawPreview(rct_drawpixelinfo* dpi, int32_t width, int32_t height) const
{
    auto screenCoords = ScreenCoordsXY{ width / 2 - 16, height / 2 };
    gfx_draw_sprite(dpi, ImageId(BaseImageId + 3), screenCoords);
    gfx_draw_sprite(dpi, ImageId(BaseImageId + 16), { screenCoords.x + 32, screenCoords.y - 16 });
    gfx_draw_sprite(dpi, ImageId(BaseImageId + 8), { screenCoords.x + 32, screenCoords.y + 16 });
}

void FootpathSurfaceObject::ReadJson(IReadObjectContext* context, json_t& root)
{
    Guard::Assert(root.is_object(), "FootpathSurfaceObject::ReadJson expects parameter root to be object");

    auto properties = root["properties"];
    if (properties.is_object())
    {
        Flags = Json::GetFlags<uint8_t>(
            properties,
            {
                { "editorOnly", FOOTPATH_ENTRY_FLAG_SHOW_ONLY_IN_SCENARIO_EDITOR },
                { "isQueue", FOOTPATH_ENTRY_FLAG_IS_QUEUE },
                { "noSlopeRailings", FOOTPATH_ENTRY_FLAG_NO_SLOPE_RAILINGS },
            });
    }

    PopulateTablesFromJson(context, root);
}

void FootpathSurfaceObject::SetRepositoryItem(ObjectRepositoryItem* item) const
{
    item->FootpathSurfaceInfo.Flags = Flags;
}
