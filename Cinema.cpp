#include "Cinema.hpp"
#include <random>
#include <vector>

Cinema::Cinema()
    : cinemaState(CinemaState::SELLING), color{255, 255, 255}, frameCounter(0)
{
    // Sva sedišta su AVAILABLE na početku
    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            seats[r][c] = SeatState::AVAILABLE;
        }
    }
}

void Cinema::ToggleSeat(int x, int y) {
    if (cinemaState == CinemaState::SELLING) {
        SeatState state = seats[x][y];
        if (state == SeatState::AVAILABLE) {
            seats[x][y] = SeatState::RESERVED;
        }
        else if (state == SeatState::RESERVED) {
            seats[x][y] = SeatState::AVAILABLE;
        }
    }
}

void Cinema::BuySeats(int number) {
    if (number < 1 || number > 9) return;

    if (cinemaState != CinemaState::SELLING) return;

    for (int r = ROWS - 1; r >= 0; r--) {

        for (int c = COLS - number; c >= 0; c--) {

            bool allAvailable = true;

            for (int k = 0; k < number; k++) {
                if (seats[r][c + k] != SeatState::AVAILABLE) {
                    allAvailable = false;
                    break;
                }
            }

            if (allAvailable) {
                for (int k = 0; k < number; k++) {
                    seats[r][c + k] = SeatState::BOUGHT;
                }
                return;
            }
        }
    }
}

void Cinema::ResetSeats()
{
    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            seats[r][c] = SeatState::AVAILABLE;
        }
    }
}

SeatState Cinema::GetSeatState(int x, int y) {
    return seats[x][y];
}

CinemaState Cinema::GetCinemaState()
{
    return cinemaState;
}

std::vector<std::pair<int, int>> Cinema::GetTakenSeats() {
    std::vector<std::pair<int, int>> taken;

    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            if (seats[r][c] == SeatState::RESERVED ||
                seats[r][c] == SeatState::BOUGHT)
            {
                taken.emplace_back(r, c);
            }
        }
    }

    return taken;
}

std::vector<std::pair<int, int>> Cinema::GetSelectedSeats()
{
    return selectedSeats;
}

void Cinema::GetRandomTakenSeats() {
    std::vector<std::pair<int, int>> taken = GetTakenSeats();

    if (taken.empty()) return;  // nema nijednog zauzetog sedišta → vrati prazno

    // RNG
    static std::random_device rd;
    static std::mt19937 gen(rd());

    // Koliko odabrati? 1 ... taken.size()
    std::uniform_int_distribution<int> countDist(1, (int)taken.size());
    int k = countDist(gen);

    // Izmešaj listu (Fisher–Yates / std::shuffle)
    std::shuffle(taken.begin(), taken.end(), gen);

    // Vrati prvih k
    selectedSeats = std::vector<std::pair<int, int>>(taken.begin(), taken.begin() + k);
}

void Cinema::SitOnSeats() {
    for (std::pair<int, int> seat : selectedSeats) {
        if (seats[seat.first][seat.second] == SeatState::RESERVED) {
            seats[seat.first][seat.second] = SeatState::RESERVED_SITTING;
        }
        else {
            seats[seat.first][seat.second] = SeatState::BOUGHT_SITTING;
        }
    }
}

void Cinema::StandUp() {
    for (std::pair<int, int> seat : selectedSeats) {
        if (seats[seat.first][seat.second] == SeatState::RESERVED_SITTING) {
            seats[seat.first][seat.second] = SeatState::RESERVED;
        }
        else if (seats[seat.first][seat.second] == SeatState::BOUGHT_SITTING) {
            seats[seat.first][seat.second] = SeatState::BOUGHT;
        }
    }
}

std::array<int, 3> Cinema::GetColor()
{
    return color;
}

void Cinema::SwitchState() {
    if (cinemaState == CinemaState::SELLING) {
        cinemaState = CinemaState::ENTERING;
    }
    else if (cinemaState == CinemaState::ENTERING) {
        cinemaState = CinemaState::PLAYING;
    }
    else if (cinemaState == CinemaState::PLAYING) {
        cinemaState = CinemaState::LEAVING;
    }
    else if (cinemaState == CinemaState::LEAVING) {
        cinemaState = CinemaState::SELLING;
    }
}

void Cinema::IncreaseFrameCounter() {
    frameCounter = (frameCounter + 1) % 20;
    if (frameCounter == 0) {
        color[0] = getRandom0toX(255);
        color[1] = getRandom0toX(255);
        color[2] = getRandom0toX(255);
    }
}

void Cinema::ResetFrameCounter() {
    frameCounter = 0;
    color[0] = 255;
    color[1] = 255;
    color[2] = 255;
}

int Cinema::getRandom0toX(int n) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<int> dist(0, n);

    return dist(gen);
}