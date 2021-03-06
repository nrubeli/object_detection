#pragma once

#include <iostream>
#include <string>
#include "base_menu.h"
#include "config_kpd_menu.h"
#include "../src/objectdetection.h"

class ConfigInputMenu : public BaseMenu
{
public:
    ConfigInputMenu(BaseMenu* menu, ParameterHandler* param_handler)
    {
        MenuName = std::string("Input Config Menu");
        m_MenuText = std::string("D - set Dataset\n")
            + "O - set Object\n"
            + "P - set Preprocessor mode\n"
            + "R - Return";
        parent = menu;
        child = true;
        parameter_handler = param_handler;
    }

    BaseMenu* getNextMenu(char choice, bool& quit, bool& execute)
    {
        BaseMenu* aNewMenu = 0;

        switch (choice)
        {
        case 'D':
        {
            parameter_handler->set_dataset(get_input());
        }
        break;
        case 'O':
        {
            parameter_handler->set_object(get_input());
        }
        break;
        case 'P':
        {
            parameter_handler->set_preprocessor_mode(get_input());
        }
        case 'R':
        {
            aNewMenu = parent;
        }
        break;
        default:
        {
            // Do nothing - we won't change the menu
        }

        }

        return aNewMenu; // Sending it back to the main function
    }
};