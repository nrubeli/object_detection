#pragma once

#include <iostream>
#include <string>
#include "base_menu.h"
#include "merge_menu.h"

class MergeMenu : public BaseMenu
{
public:
    MergeMenu()
    {
        MenuName = std::string("MergeMenu\n");
        m_MenuText = std::string("Please make your selection\n")
            + std::string("Please make your selection\n")
            + std::string("a - Return to MainMenu\n")
            + std::string("b - Return to MainMenu");
    }

    BaseMenu* getNextMenu(char choice, bool& quit, bool& execute)
    {
        BaseMenu* aNewMenu = 0;

        switch (choice)
        {
        case 'a':
        {
            std::cout << "pushed 1" << std::endl;
        }
        break;
        case 'b':
        {
            std::cout << "pushed 2" << std::endl;
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