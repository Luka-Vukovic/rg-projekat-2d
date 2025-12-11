#ifndef CINEMA_HPP
#define CINEMA_HPP

#include <array>
#include <vector>

enum class SeatState {
    AVAILABLE = 0,
    RESERVED = 1,
    RESERVED_SITTING = 2,
    BOUGHT = 3,
    BOUGHT_SITTING = 4,
};

enum class CinemaState {
    SELLING = 0,
    ENTERING = 1,
    PLAYING = 2,
    LEAVING = 3,
};

class Cinema {
private:
    // Matrica sedišta 6x12
    static const int ROWS = 6;
    static const int COLS = 12;

    std::array<std::array<SeatState, COLS>, ROWS> seats;

    std::vector<std::pair<int, int>> selectedSeats;

    CinemaState cinemaState;

    // Boja RGB (0–255)
    std::array<int, 3> color;

    int frameCounter;

public:
    // Konstruktor
    Cinema();

    // Metode
    void ToggleSeat(int x, int y);
    void BuySeats(int number);
    void ResetSeats();
    SeatState GetSeatState(int x, int y);
    CinemaState GetCinemaState();
    std::vector<std::pair<int, int>> GetTakenSeats();
    std::vector<std::pair<int, int>> GetSelectedSeats();
    void GetRandomTakenSeats();
    void SitOnSeats();
    void StandUp();
    std::array<int, 3> GetColor();
    void SwitchState();
    void IncreaseFrameCounter();
    void ResetFrameCounter();
    int getRandom0toX(int n);
};

#endif
#pragma once


