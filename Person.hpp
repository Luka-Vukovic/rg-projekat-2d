#ifndef PERSON_HPP
#define PERSON_HPP

enum class PersonState {
    WALKING_TO_SEAT,
    SITTING,
    WALKING_TO_EXIT,
    EXITED
};

struct Person {
    int seatRow;
    int seatCol;

    float doorPosX, doorPosY;
    float seatPosX, seatPosY;

    float uX = 0.0f;
    float uY = 0.0f;

    PersonState state;
    float speed = 0.6f;
    bool movingToY = true;

    unsigned int texture;
};

#endif