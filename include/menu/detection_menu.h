#pragma once

#include <iostream>
#include <string>
#include "base_menu.h"
#include "config_kpd_menu.h"
#include "config_normal_estimator_menu.h"
#include "config_output_menu.h"
#include "../src/objectdetection.h"

class DetectionMenu : public BaseMenu
{
public:
    DetectionMenu()
    {
        MenuName = std::string("Detection Menu");
        m_MenuText = std::string("D - set Dataset\n")
            + "O - set Object\n"
            + "P - set Preprocessor mode\n"
            + "C - set deteCtion threshold\n"
            + "N - Configure Normal estimator\n"
            + "T - Configure KPD\n"
            + "V - Configure output\n"
            + "E - Execute\n"
            + "Q - Quit";
    }

    BaseMenu* getNextMenu(char choice, bool& quit, bool& execute)
    {
        BaseMenu* aNewMenu = 0;

        switch (choice)
        {
        case 'D':
        {
            set_dataset(get_input());
        }
        break;
        case 'O':
        {
            set_object(get_input());
        }
        break;
        case 'C':
        {
            set_detection_threshold(std::stoi(get_input()));
        }
        break;
        case 'N':
        {
            aNewMenu = new ConfigNEMenu(this);
        }
        break;
        case 'P':
        {
            set_preprocessor(get_input());
        }
        break;
        case 'T':
        {
            aNewMenu = new ConfigKPDMenu(this);
        }
        break;
        case 'V':
        {
            aNewMenu = new ConfigOutputMenu(this);
        }
        break;
        case 'E':
        {
            set_execution_param(std::string("detection"));
            execute = true;
        }
        break;
        case 'Q':
        {
            quit = true;
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