/*****************************************************************************
 * Copyright (c) 2014-2019 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include "../Context.h"
#include "../management/Finance.h"
#include "../windows/Intent.h"
#include "../world/Banner.h"
#include "GameAction.h"

// There is also the BannerSetColourAction that sets primary colour but this action takes banner index rather than x, y, z,
// direction
enum class BannerSetStyleType : uint8_t
{
    PrimaryColour,
    TextColour,
    NoEntry,
    Count
};

DEFINE_GAME_ACTION(BannerSetStyleAction, GAME_COMMAND_SET_BANNER_STYLE, GameActionResult)
{
private:
    uint8_t _type = static_cast<uint8_t>(BannerSetStyleType::Count);
    uint8_t _bannerIndex = BANNER_INDEX_NULL;
    uint8_t _parameter;

public:
    BannerSetStyleAction() = default;

    BannerSetStyleAction(BannerSetStyleType type, uint8_t bannerIndex, uint8_t parameter)
        : _type(static_cast<uint8_t>(type))
        , _bannerIndex(bannerIndex)
        , _parameter(parameter)
    {
    }

    uint16_t GetActionFlags() const override
    {
        return GameAction::GetActionFlags() | GAME_COMMAND_FLAG_ALLOW_DURING_PAUSED;
    }

    void Serialise(DataSerialiser & stream) override
    {
        GameAction::Serialise(stream);

        stream << DS_TAG(_type) << DS_TAG(_bannerIndex) << DS_TAG(_parameter);
    }

    GameActionResult::Ptr Query() const override
    {
        auto res = MakeResult();

        if (_bannerIndex >= MAX_BANNERS || _bannerIndex == BANNER_INDEX_NULL)
        {
            return MakeResult(GA_ERROR::INVALID_PARAMETERS, STR_INVALID_SELECTION_OF_OBJECTS);
        }

        rct_banner* banner = &gBanners[_bannerIndex];

        res->ExpenditureType = RCT_EXPENDITURE_TYPE_LANDSCAPING;
        res->Position.x = banner->x * 32 + 16;
        res->Position.y = banner->y * 32 + 16;
        res->Position.z = tile_element_height(banner->x, banner->y) & 0xFFFF;

        TileElement* tileElement = banner_get_tile_element(_bannerIndex);

        if (tileElement == nullptr)
        {
            log_error("Could not find banner index = %u", _bannerIndex);
            return MakeResult(GA_ERROR::INVALID_PARAMETERS, STR_NONE);
        }

        switch (static_cast<BannerSetStyleType>(_type))
        {
            case BannerSetStyleType::PrimaryColour:
                if (_parameter > 31)
                {
                    log_error("Invalid primary colour: colour = %u", _parameter);
                    return MakeResult(GA_ERROR::INVALID_PARAMETERS, STR_CANT_REPAINT_THIS);
                }
                break;

            case BannerSetStyleType::TextColour:
                if (_parameter > 13)
                {
                    log_error("Invalid text colour: colour = %u", _parameter);
                    return MakeResult(GA_ERROR::INVALID_PARAMETERS, STR_CANT_REPAINT_THIS);
                }
                break;
            case BannerSetStyleType::NoEntry:
                break;
            default:
                log_error("Invalid type: %u", _type);
                return MakeResult(GA_ERROR::INVALID_PARAMETERS, STR_NONE);
        }
        return res;
    }

    GameActionResult::Ptr Execute() const override
    {
        auto res = MakeResult();

        rct_banner* banner = &gBanners[_bannerIndex];

        res->ExpenditureType = RCT_EXPENDITURE_TYPE_LANDSCAPING;
        res->Position.x = banner->x * 32 + 16;
        res->Position.y = banner->y * 32 + 16;
        res->Position.z = tile_element_height(banner->x, banner->y) & 0xFFFF;

        TileElement* tileElement = banner_get_tile_element(_bannerIndex);

        if (tileElement == nullptr)
        {
            log_error("Could not find banner index = %u", _bannerIndex);
            return MakeResult(GA_ERROR::INVALID_PARAMETERS, STR_NONE);
        }

        switch (static_cast<BannerSetStyleType>(_type))
        {
            case BannerSetStyleType::PrimaryColour:
                banner->colour = _parameter;
                break;
            case BannerSetStyleType::TextColour:
            {
                banner->text_colour = _parameter;
                int32_t colourCodepoint = FORMAT_COLOUR_CODE_START + banner->text_colour;

                utf8 buffer[256];
                format_string(buffer, 256, banner->string_idx, nullptr);
                int32_t firstCodepoint = utf8_get_next(buffer, nullptr);
                if (firstCodepoint >= FORMAT_COLOUR_CODE_START && firstCodepoint <= FORMAT_COLOUR_CODE_END)
                {
                    utf8_write_codepoint(buffer, colourCodepoint);
                }
                else
                {
                    utf8_insert_codepoint(buffer, colourCodepoint);
                }

                rct_string_id stringId = user_string_allocate(USER_STRING_DUPLICATION_PERMITTED, buffer);
                if (stringId != 0)
                {
                    rct_string_id prevStringId = banner->string_idx;
                    banner->string_idx = stringId;
                    user_string_free(prevStringId);
                }
                else
                {
                    return MakeResult(GA_ERROR::INVALID_PARAMETERS, STR_ERR_CANT_SET_BANNER_TEXT);
                }
                break;
            }
            case BannerSetStyleType::NoEntry:
            {
                BannerElement* bannerElement = tileElement->AsBanner();
                banner->flags &= BANNER_FLAG_NO_ENTRY;
                banner->flags |= BANNER_FLAG_NO_ENTRY & (_parameter != 0);
                uint8_t allowedEdges = 0xF;
                if (banner->flags & BANNER_FLAG_NO_ENTRY)
                {
                    allowedEdges &= ~(1 << bannerElement->GetPosition());
                }
                bannerElement->SetAllowedEdges(allowedEdges);
                break;
            }
            default:
                log_error("Invalid type: %u", _type);
                return MakeResult(GA_ERROR::INVALID_PARAMETERS, STR_NONE);
        }

        auto intent = Intent(INTENT_ACTION_UPDATE_BANNER);
        intent.putExtra(INTENT_EXTRA_BANNER_INDEX, _bannerIndex);
        context_broadcast_intent(&intent);

        return res;
    }
};
