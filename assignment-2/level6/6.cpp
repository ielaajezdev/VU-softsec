#include <iostream>
#include <vector>

#include "noaslr.h"

#include "cooking.h"

void add_base_ingredients(std::vector<Ingredient *> *ingredient_list) {
    ingredient_list->push_back(new BasicIngredient((char *)"Pork Meat", 200, MEAT));
    ingredient_list->push_back(new BasicIngredient((char *)"Chicken Meat", 150, MEAT));
    ingredient_list->push_back(new BasicIngredient((char *)"Lettuce", 50, VEGETABLE));
    ingredient_list->push_back(new BasicIngredient((char *)"Carrots", 80, VEGETABLE));
    ingredient_list->push_back(new BasicIngredient((char *)"Spinach", 120, VEGETABLE));
    ingredient_list->push_back(new BasicIngredient((char *)"Beets", 420, VEGETABLE));
    ingredient_list->push_back(new Spice((char *)"Cinnamon", (char *) "Tastes funny"));
    ingredient_list->push_back(new Spice((char *)"Salt", (char *) "Smells like the ocean"));
}

void list_ingredients(std::vector<Ingredient *> ingredient_list) {
	std::cout << "Ingredients:" << std::endl;
    size_t idx = 0;
    for (const auto ingredient : ingredient_list) {
        std::cout << idx++ << ": " << ingredient->GetName() << std::endl;
    }
}

void cook(std::vector<Ingredient *> *ingredient_list) {
    Dish *dish = new Dish();
    BasicIngredient *ingredient = nullptr;
    Spice *spice = nullptr;
    size_t p1, p2;
    std::string input, temp;
    bool running = true;

    while (running) {
        p1 = p2 = 0;
        std::cout << "> ";
        std::getline(std::cin, input);

        switch (input[0]) {
            case 'l':
                list_ingredients(*ingredient_list);
                break;

            case 'a':
                if (input.size() != 3) {
                    std::cout << "Invalid command format" << std::endl;
                    break;
                }

                try {
                    p1 = std::stoi(input.substr(2, 1));
                    if (p1 < 0 || p1 >= ingredient_list->size()) {
                        std::cout << "Invalid ingredient id" << std::endl;
                        break;
                    }

                    dish->AddIngredient(ingredient_list->at(p1));
                } catch (const std::invalid_argument& e) {
                    std::cout << "Invalid ingredient id" << std::endl;
                }
                break;

            case 'c':
                if (input.size() < 5 || (input.size() >= 5 && input.at(3) != '=')) {
                    std::cout << "Invalid command format" << std::endl;
                    break;
                }

                try {
                    p1 = std::stoi(input.substr(2, 1));
                    if (p1 < 0 || p1 >= ingredient_list->size()) {
                        std::cout << "Invalid ingredient id" << std::endl;
                        break;
                    }

                    p2 = std::stoull(input.substr(4));

                    ingredient = static_cast<BasicIngredient *> (ingredient_list->at(p1));
                    std::cout << "Changing caloric content of " << ingredient->GetName() << 
                        " to " << p2 << std::endl;

                    ingredient->SetCalories((size_t) p2);
                    ingredient = nullptr;
                } catch (const std::invalid_argument& e) {
                    std::cout << "Invalid ingredient id or caloric value" << std::endl;
                }

                break;

            case 'e':
                if (input.size() < 5 || (input.size() >= 5 && input.at(3) != '=')) {
                    std::cout << "Invalid command format" << std::endl;
                    break;
                }

                try {
                    p1 = std::stoi(input.substr(2, 1));
                    if (p1 < 0 || p1 >= ingredient_list->size()) {
                        std::cout << "Invalid ingredient id" << std::endl;
                        break;
                    }

                    if (ingredient_list->at(p1)->GetType() != SPICE) {
                        std::cout << "Can only change effect of spices" << std::endl;
                        break;
                    }

                    temp = input.substr(4);
                    spice = static_cast<Spice *> (ingredient_list->at(p1));
                    std::cout << "Changing effect of " << spice->GetName() <<
                        " to " << temp << std::endl;

                    spice->SetSpecialEffect(temp.c_str());
                    spice = nullptr;
                } catch (const std::invalid_argument& e) {
                    std::cout << "Invalid ingredient id or caloric value" << std::endl;
                }

                break;

            case 'h':
                if (input.size() <= 2) {
                    std::cout << "Invalid command format" << std::endl;
                    break;
                }

                try {
                    p1 = std::stoi(input.substr(2));
                    std::cout << "Heating to " << p1 << " degrees C" << std::endl;
                    if (dish->Heat(p1) == false) {
                        std::cout << "Error: Temperature outside of oven range" << std::endl;
                    }
                } catch (const std::invalid_argument& e) {
                    std::cout << "Invalid temperature" << std::endl;
                }
                break;

            case 'f':
                dish->Complete();
                free(dish);
                dish = new Dish();
                std::cout << std::endl;

            case 'n':
                std::cout << "Starting new dish ..." << std::endl;
                free(dish);
                dish = new Dish();
                break;

            case 'q':
                std::cout << "Goodbye" << std::endl;
                running = false;
                break;

            default:
                std::cout << "Invalid input" << std::endl;
                break;
        }
    }

    free(dish);
}

int main(int argc, char **argv, char **envp) {
    std::vector<Ingredient *> ingredient_list;
    add_base_ingredients(&ingredient_list);

    std::cout << "Available Commands:" << std::endl;
    std::cout << "- l               | List ingredients" << std::endl;
    std::cout << "- a id            | Add ingredient to dish" << std::endl;
    std::cout << "- c id=value      | Change caloric content of ingredient" << std::endl;
    std::cout << "- e id=string     | Change effect of spice" << std::endl;
    std::cout << "- h temp          | Heat the current dish to target temp" << std::endl;
    std::cout << "- f               | Finish cooking current dish" << std::endl;
    std::cout << "- n               | Create new dish" << std::endl;
    std::cout << "- q               | Exit program" << std::endl;
    std::cout << std::endl;

    cook(&ingredient_list);

    return 0;
}
