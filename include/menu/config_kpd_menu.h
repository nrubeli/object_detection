#pragma once

#include <iostream>
#include <string>
#include "base_menu.h"
#include "../src/objectdetection.h"

class ConfigKPDMenu : public BaseMenu
{
public:
    ConfigKPDMenu(BaseMenu* menu, ParameterHandler* param_handler)
    {
        MenuName = std::string("Config KPD Menu");
        m_MenuText = std::string("T - add KPD Threshold(default = 0.700)\n")
            + "N - add KPD #Neighbors (default = 5)\n"
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
        case 'T':
        {
            parameter_handler->add_detector_threshold(std::stof(get_input().substr(0, 5)));
        }
        break;
        case 'N':
        {
            parameter_handler->add_detector_nn(std::stoi(get_input()));
        }
        break;
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