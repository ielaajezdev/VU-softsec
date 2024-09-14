#include "cooking.h"

#include <iostream>

std::string StateToString(IngredientState state) {
    return state == RAW ? "raw" : state == COOKED ? "cooked" : "burned";
}

bool Dish::Heat(unsigned int temp) {
    if (this->finished || temp < 0 || temp > 1000) {
        return false;
    }

    if (this->meat_cnt > 0 && temp > 130) {
        if (this->meat_state == RAW) {
            this->meat_state = COOKED;
        }

        if (temp > 200) {
            this->meat_state = BURNED;
        }
    }

    if (this->veggie_cnt > 0 && temp > 120) {
        if (this->veggie_state == RAW) {
            this->veggie_state = COOKED;
        }

        if (temp > 150) {
            this->veggie_state = BURNED;
        }
    }

    return true;
}

bool Dish::AddIngredient(Ingredient *ingredient) {
    if (this->finished) {
        return false;
    }

    if (ingredient->GetType() == MEAT) {
        this->calories += static_cast<BasicIngredient *>(ingredient)->GetCalories();
        this->meat_cnt++;
    } else if (ingredient->GetType() == VEGETABLE) {
        this->calories += static_cast<BasicIngredient *>(ingredient)->GetCalories();
        this->veggie_cnt++;
    } else {
        this->spice = (Spice *) ingredient;
    }

    return true;
}

void Dish::Complete() {
    if (this->finished) {
        return;
    }

    this->finished = true;

    std::cout << "You have created a beautiful ";
    if (this->meat_cnt == 0 && this->veggie_cnt == 0) {
        std::cout << "dish consisting of ";
        if (this->spice)  {
            std::cout << "only spice";
        } else {
            std::cout << "nothing";
        }
        std::cout << std::endl;
        return;
    }

    if (this->meat_cnt > this->veggie_cnt) {
        std::cout << StateToString(this->meat_state) << " meat dish";
        if (this->veggie_cnt) {
            std::cout << " with some " << StateToString(this->veggie_state)
                << " vegetables";
        }
    } else {
        std::cout << StateToString(this->veggie_state) << " vegetable dish";
        if (this->meat_cnt) {
            std::cout << " with some " << StateToString(this->meat_state)
                << " meat";
        }
    }
    std::cout << std::endl;

    if (this->spice && this->spice->GetSpecialEffect()) {
        std::cout << "- Spice: " << this->spice->GetName() << " ("
            << this->spice->GetSpecialEffect() << ")" << std::endl;
    }

    std::cout << "- Calories: " << this->calories << std::endl;
}
