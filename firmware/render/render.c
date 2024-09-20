/*
MIT License

Copyright (c) 2021 Mark Aikens
Copyright (c) 2023 David Kuder
Copyright (c) 2024 Thorsten Brehm

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <stdlib.h>
#include "applebus/buffers.h"
#include "config/config.h"

#include "render.h"
#include "menu/menu.h"

bool mono_rendering = false;
bool color_support;

void DELAYED_COPY_CODE(render_init)()
{
    // clear status lines
    for (uint i=0;i<sizeof(status_line)/4;i++)
    {
        ((uint32_t*)status_line)[i] = 0xA0A0A0A0;
    }
}

void DELAYED_COPY_CODE(cycle_display_modes)()
{
    if (color_support)
    {
        if (IS_IFLAG(IFLAGS_FORCED_MONO))
        {
            if (color_mode < COLOR_MODE_AMBER)
                color_mode++;
            else
            {
                color_mode = COLOR_MODE_BW;
                SET_IFLAG(false, IFLAGS_FORCED_MONO);
            }
        }
        else
        {
            SET_IFLAG(!IS_IFLAG(IFLAGS_SCANLINEEMU), IFLAGS_SCANLINEEMU);
            SET_IFLAG(true, IFLAGS_FORCED_MONO);
        }
    }
    else
    {
        if (color_mode < COLOR_MODE_AMBER)
            color_mode++;
        else
        {
            color_mode = COLOR_MODE_BW;
            SET_IFLAG(!IS_IFLAG(IFLAGS_SCANLINEEMU), IFLAGS_SCANLINEEMU);
        }
    }
}

// check if the button was toggled quickly (to trigger extra functions)
bool DELAYED_COPY_CODE(quick_button_toggle())
{
    static uint32_t last_ms = 0;

    // get current time
    uint32_t current_ms = to_ms_since_boot(get_absolute_time());

    // was button state toggled quickly?
    bool quick = ((last_ms!=0)&&(current_ms - last_ms < 1000));

    // remember current time
    last_ms = current_ms;

    return quick;
}

// check button state and trigger configured actions
void DELAYED_COPY_CODE(update_toggle_switch)()
{
    static uint debounce_counter;
    static bool debounce_state;

    if ((input_switch_state != debounce_state)||
        (input_switch_mode == ModeSwitchDisabled))
    {
        debounce_counter = 0;
        debounce_state = input_switch_state;
        return;
    }
    debounce_counter++;
    if (debounce_counter < 5)
        return;
    if (debounce_counter > 5)
    {
        debounce_counter = 6; // avoid overflows
        return;
    }

    switch(input_switch_mode)
    {
        case ModeSwitchLanguage:
            // Default Euro-Machine behavior:
            // switch state directly controls character set selection.
            language_switch = input_switch_state;
            break;

        case ModeSwitchMonochrome:
            // Simple switch behavior for US-charset machines:
            // Switch directly controls color vs monochrome mode.
            SET_IFLAG(input_switch_state, IFLAGS_FORCED_MONO);
            break;

        case ModeSwitchCycleVideo:
            // More complex behavior for US-charset machines:
            // Switch cycles through various display modes.
            // (Using a push-button is a good choice for this mode).
            if (input_switch_state)
            {
                cycle_display_modes();
            }
            break;

        case ModeSwitchLangMonochrome:
            // Advanced switch behavior for Euro-Machines:
            // State of the language switch directly selects the character set.
            // Toggling the switch quickly (in less than 1 second) toggles
            // between color and monochrome mode.
            language_switch = input_switch_state;
            if (quick_button_toggle())
            {
                // toggle monochrome mode
                SET_IFLAG(!IS_IFLAG(IFLAGS_FORCED_MONO), IFLAGS_FORCED_MONO);
            }
            break;

        case ModeSwitchLangCycle:
        {
            // More advanced switch behavior for Euro-Machines:
            // State of the language switch directly selects the character set.
            // Toggling the switch quickly (in less than 1 second) cycles
            // through various display modes.
            language_switch = input_switch_state;
            if (quick_button_toggle())
            {
                cycle_display_modes();
            }
            break;
        }

        case ModeSwitchDisabled: // fall-through
        default:
            break;
    }

    // when cycling through the video modes, we need to update the menu when it's on display
    if ((input_switch_mode != ModeSwitchLanguage) && (IS_IFLAG(IFLAGS_MENU_ENABLE)))
    {
        menuShow(1);
    }
}

void DELAYED_COPY_CODE(render_loop)()
{
    render_init();

    for(;;)
    {
        render_debug(true);

        // set flag when monochrome rendering is requested
        mono_rendering = (soft_switches & SOFTSW_MONOCHROME)||(internal_flags & IFLAGS_FORCED_MONO);

        // prepare state indicating whether the current display mode supports colors
        color_support = (soft_switches & SOFTSW_MONOCHROME) ? false : true;

        switch(soft_switches & SOFTSW_MODE_MASK)
        {
            case 0:
                if(soft_switches & SOFTSW_DGR)
                {
                    render_dgr();
                }
                else
                {
                    render_lores();
                }
                break;
            case SOFTSW_MIX_MODE:
                if((soft_switches & (SOFTSW_80COL | SOFTSW_DGR)) == (SOFTSW_80COL | SOFTSW_DGR))
                {
                    render_mixed_dgr();
                }
                else
                {
                    render_mixed_lores();
                }
                break;
            case SOFTSW_HIRES_MODE:
                if(soft_switches & SOFTSW_DGR)
                {
                    render_dhgr();
                }
                else
                {
                    render_hires();
                }
                break;
            case SOFTSW_HIRES_MODE|SOFTSW_MIX_MODE:
                if((soft_switches & (SOFTSW_80COL | SOFTSW_DGR)) == (SOFTSW_80COL | SOFTSW_DGR))
                {
                    render_mixed_dhgr();
                }
                else
                {
                    render_mixed_hires();
                }
                break;
            default:
                render_text();
                break;
        }

        render_debug(false);

        update_text_flasher();

        update_toggle_switch();

        dvi0.scanline_emulation = (internal_flags & IFLAGS_SCANLINEEMU) != 0;

        frame_counter++;
    }
}
