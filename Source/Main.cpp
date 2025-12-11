#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <random>

#include "../Header/Util.h"
#include "../Cinema.hpp"
#include "../Person.hpp"

int screenWidth = 800;
int screenHeight = 800;
Cinema cinema = Cinema();

double enteringStart = NULL;
double playingStart = NULL;
double leavingStart = NULL;

unsigned seatTextures[5];

std::vector<Person> people;
unsigned int VAOperson, VBOperson;
unsigned int personTexture;
bool peopleInitialized = false;

unsigned int watermarkTexture;

GLFWcursor* cursor;

#include <vector>

struct SeatBounds {
    float x1, y1; // donji levi ugao
    float x2, y2; // gornji desni ugao
};

std::vector<SeatBounds> seatBounds;

std::vector<float> GenerateSeatQuads(
    float startX,
    float startY,
    float seatHeight,
    float aspectRatio,
    float horizontalGap,
    float verticalGap
) {
    const int ROWS = 6;
    const int COLS = 12;

    float seatWidth = seatHeight * aspectRatio;

    std::vector<float> data;
    data.reserve(ROWS * COLS * 16);

    seatBounds.clear();
    seatBounds.reserve(ROWS * COLS);

    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {

            float x = startX + c * (seatWidth + horizontalGap);
            float y = startY - r * (seatHeight + verticalGap);

            float x1 = x;
            float y1 = y;

            float x2 = x + seatWidth;
            float y2 = y;

            float x3 = x + seatWidth;
            float y3 = y - seatHeight;

            float x4 = x;
            float y4 = y - seatHeight;

            data.insert(data.end(), {
                x1, y1, 0.0f, 1.0f,
                x2, y2, 1.0f, 1.0f,
                x3, y3, 1.0f, 0.0f,
                x4, y4, 0.0f, 0.0f
                });

            SeatBounds bounds;
            bounds.x1 = x1;          // leva granica
            bounds.y1 = y4;          // donja granica (jer y ide na dole)
            bounds.x2 = x2;          // desna granica
            bounds.y2 = y1;          // gornja granica
            seatBounds.push_back(bounds);
        }
    }

    return data;
}

// Funkcija za konverziju koordinata miša u OpenGL koordinate
void ScreenToOpenGLCoords(double screenX, double screenY, float& glX, float& glY) {
    // Konvertuj iz koordinata prozora u OpenGL koordinate (-1 do 1)
    glX = (2.0f * screenX) / screenWidth - 1.0f;
    glY = 1.0f - (2.0f * screenY) / screenHeight; // Y je obrnut u OpenGL-u
}

// Provera da li je klik unutar sedišta
bool IsClickOnSeat(float glX, float glY, int& row, int& col) {
    const int ROWS = 6;
    const int COLS = 12;

    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            int index = r * COLS + c;
            const SeatBounds& bounds = seatBounds[index];

            // Proveri da li su koordinate unutar granica sedišta
            if (glX >= bounds.x1 && glX <= bounds.x2 &&
                glY >= bounds.y1 && glY <= bounds.y2) {
                row = r;
                col = c;
                return true;
            }
        }
    }
    return false;
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        double mouseX, mouseY;
        glfwGetCursorPos(window, &mouseX, &mouseY);

        float glX, glY;
        ScreenToOpenGLCoords(mouseX, mouseY, glX, glY);

        int row, col;
        if (IsClickOnSeat(glX, glY, row, col)) {
            cinema.ToggleSeat(row, col);
        }
    }
}

bool numberKeysActive[10] = { false };

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    for (int i = 1; i <= 9; i++) {
        if (key == GLFW_KEY_0 + i || key == GLFW_KEY_KP_0 + i) {
            if (action == GLFW_PRESS && !numberKeysActive[i]) {
                cinema.BuySeats(i);
                numberKeysActive[i] = true;
            }
            else if (action == GLFW_RELEASE) {
                numberKeysActive[i] = false;
            }
        }
    }

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }

    if (key == GLFW_KEY_ENTER && action == GLFW_PRESS) {
        if (enteringStart == NULL && playingStart == NULL && leavingStart == NULL) {
            enteringStart = glfwGetTime();
            cinema.SwitchState();
            cinema.GetRandomTakenSeats();
            peopleInitialized = false;
        }
    }
}

void preprocessTexture(unsigned& texture, const char* filepath) {
    texture = loadImageToTexture(filepath);
    glBindTexture(GL_TEXTURE_2D, texture);

    // PRVO podesite parametre, ONDA generišite mipmape
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Generišite mipmape NAKON što ste podesili parametre
    glGenerateMipmap(GL_TEXTURE_2D);

    // Ovo je OPCIONALNO: proverite da li je tekstura kompletna
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 4);
}

void formSeatsVAO(float* vertices, size_t verticesSize, unsigned int& VAOseats, unsigned int& VBOseats)
{
    glGenVertexArrays(1, &VAOseats);
    glGenBuffers(1, &VBOseats);

    glBindVertexArray(VAOseats);

    glBindBuffer(GL_ARRAY_BUFFER, VBOseats);
    glBufferData(GL_ARRAY_BUFFER, verticesSize, vertices, GL_STATIC_DRAW);

    // layout(location = 0) = vec2 aPos;
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // layout(location = 1) = vec2 aTex;
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

void formScreenVAO(unsigned int& VAOscreen, unsigned int& VBOscreen)
{
    float vertices[] = {
         -0.4f, 0.8f, 0.0f, 0.0f, 0.0f, // gornje levo teme
         -0.4f, 0.6f, 0.0f, 0.0f, 0.0f, // donje levo teme
         0.6f, 0.6f, 0.0f, 0.0f, 0.0f, // donje desno teme
         0.6f, 0.8f, 0.0f, 0.0f, 0.0f, // gornje desno teme
    };

    glGenVertexArrays(1, &VAOscreen);
    glGenBuffers(1, &VBOscreen);

    glBindVertexArray(VAOscreen);

    glBindBuffer(GL_ARRAY_BUFFER, VBOscreen);
    glBufferData(GL_ARRAY_BUFFER, 4 * 5 * sizeof(float), vertices, GL_STATIC_DRAW);

    // layout(location = 0) = vec2 aPos;
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // layout(location = 1) = vec2 aTex;
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

void formWatermarkVAO(unsigned int& VAOwatermark, unsigned int& VBOwatermark)
{
    float x1 = -0.96f;
    float y1 = -0.75f;

    float vertices[] = {
         x1, y1, 0.0f, 1.0f, // gornje levo teme
         x1, y1 - 0.1852f, 0.0f, 0.0f, // donje levo teme
         x1 + 0.3125f, y1 - 0.1852f, 1.0f, 0.0f, // donje desno teme 
         x1 + 0.3125f, y1, 1.0f, 1.0f, // gornje desno teme
    };

    glGenVertexArrays(1, &VAOwatermark);
    glGenBuffers(1, &VBOwatermark);

    glBindVertexArray(VAOwatermark);

    glBindBuffer(GL_ARRAY_BUFFER, VBOwatermark);
    glBufferData(GL_ARRAY_BUFFER, 4 * 4 * sizeof(float), vertices, GL_STATIC_DRAW);

    // layout(location = 0) = vec2 aPos;
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // layout(location = 1) = vec2 aTex;
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

void formDarkVAO(unsigned int& VAOdark, unsigned int& VBOdark)
{
    float vertices[] = {
         -1.0f, 1.0f, 0.2f, 0.2f, 0.2f, 0.5f, // gornje levo teme
         -1.0f, -1.0f, 0.2f, 0.2f, 0.2f, 0.5f, // donje levo teme
         1.0f, -1.0f, 0.2f, 0.2f, 0.2f, 0.5f, // donje desno teme
         1.0f, 1.0f, 0.2f, 0.2f, 0.2f, 0.5f, // gornje desno teme
    };

    glGenVertexArrays(1, &VAOdark);
    glGenBuffers(1, &VBOdark);

    glBindVertexArray(VAOdark);

    glBindBuffer(GL_ARRAY_BUFFER, VBOdark);
    glBufferData(GL_ARRAY_BUFFER, 4 * 6 * sizeof(float), vertices, GL_STATIC_DRAW);

    // layout(location = 0) = vec2 aPos;
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // layout(location = 1) = vec2 aTex;
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

void formDoorVAO(unsigned int& VAOdoor, unsigned int& VBOdoor, float aspect)
{
    float verticesClosed[] = {
         -0.9f, 1.0f, 0.4f, 0.2f, 0.1f, 1.0f, // gornje levo teme
         -0.9f, 0.95f, 0.4f, 0.2f, 0.1f, 1.0f, // donje levo teme
         -0.6f, 0.95f, 0.4f, 0.2f, 0.1f, 1.0f, // donje desno teme
         -0.6f, 1.0f, 0.4f, 0.2f, 0.1f, 1.0f, // gornje desno teme
    };

    float verticesOpen[] = {
         -0.9f, 1.0f, 0.4f, 0.2f, 0.1f, 1.0f, // gornje levo teme
         -0.9f, 0.466f, 0.4f, 0.2f, 0.1f, 1.0f, // donje levo teme
         -0.928f, 0.466f, 0.4f, 0.2f, 0.1f, 1.0f, // donje desno teme
         -0.928f, 1.0f, 0.4f, 0.2f, 0.1f, 1.0f, // gornje desno teme
    };

    glGenVertexArrays(1, &VAOdoor);
    glGenBuffers(1, &VBOdoor);

    glBindVertexArray(VAOdoor);

    glBindBuffer(GL_ARRAY_BUFFER, VBOdoor);
    if (cinema.GetCinemaState() == CinemaState::ENTERING || cinema.GetCinemaState() == CinemaState::LEAVING) {
        glBufferData(GL_ARRAY_BUFFER, 4 * 6 * sizeof(float), verticesOpen, GL_STATIC_DRAW);
    }
    else {
        glBufferData(GL_ARRAY_BUFFER, 4 * 6 * sizeof(float), verticesClosed, GL_STATIC_DRAW);
    }

    // layout(location = 0) = vec2 aPos;
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // layout(location = 1) = vec2 aTex;
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

void drawSeats(unsigned shader, unsigned int VAOseats)
{
    glUseProgram(shader);
    glBindVertexArray(VAOseats);

    // Izvuci lokaciju jednom
    GLint uTexLoc = glGetUniformLocation(shader, "uTex");
    glUniform1i(uTexLoc, 0); // Podesi da shader čita sa Texture Unit 0

    int vertexOffset = 0;

    for (int r = 0; r < 6; r++)
    {
        for (int c = 0; c < 12; c++)
        {
            SeatState state = cinema.GetSeatState(r, c);

            glActiveTexture(GL_TEXTURE0);
            // Ovde samo vezuješ teksturu, shader već zna da čita sa 0
            glBindTexture(GL_TEXTURE_2D, seatTextures[static_cast<int>(state)]);

            glDrawArrays(GL_TRIANGLE_FAN, vertexOffset, 4);

            vertexOffset += 4;
        }
    }

    glBindVertexArray(0);
    glUseProgram(0);
}

void drawScreen(unsigned shader, unsigned int VAOscreen)
{
    glUseProgram(shader);
    glBindVertexArray(VAOscreen);

    // Izvuci lokaciju jednom
    GLint uRLoc = glGetUniformLocation(shader, "uR");
    glUniform1f(uRLoc, ((float) cinema.GetColor()[0]) / 255.0f);
    GLint uGLoc = glGetUniformLocation(shader, "uG");
    glUniform1f(uGLoc, ((float) cinema.GetColor()[1]) / 255.0f);
    GLint uBLoc = glGetUniformLocation(shader, "uB");
    glUniform1f(uBLoc, ((float) cinema.GetColor()[2]) / 255.0f);

    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    glBindVertexArray(0);
    glUseProgram(0);
}

void drawDark(unsigned shader, unsigned int VAOdark)
{
    glUseProgram(shader);
    glBindVertexArray(VAOdark);

    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    glBindVertexArray(0);
    glUseProgram(0);
}

void drawDoor(unsigned shader, unsigned int VAOdoor)
{
    glUseProgram(shader);
    glBindVertexArray(VAOdoor);

    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    glBindVertexArray(0);
    glUseProgram(0);
}

void drawWatermark(unsigned shader, unsigned int VAOwatermark)
{
    glUseProgram(shader);
    glBindVertexArray(VAOwatermark);

    // Izvuci lokaciju jednom
    GLint uTexLoc = glGetUniformLocation(shader, "uTex");
    glUniform1i(uTexLoc, 0); // Podesi da shader čita sa Texture Unit 0

    glActiveTexture(GL_TEXTURE0);
    // Ovde samo vezuješ teksturu, shader već zna da čita sa 0
    glBindTexture(GL_TEXTURE_2D, watermarkTexture);

    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    glBindVertexArray(0);
    glUseProgram(0);
}

void InitializePeople(float seatWidth, float seatHeight, float aspect) {
    people.clear();

    std::vector<std::pair<int, int>> selectedSeats;
    selectedSeats = cinema.GetSelectedSeats();

    if (selectedSeats.empty()) return;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> doorDist(-0.88f, -0.62f);

    for (const auto& seat : selectedSeats) {
        Person person;
        person.seatRow = seat.first;
        person.seatCol = seat.second;
        person.state = PersonState::WALKING_TO_SEAT;
        person.movingToY = true;
        person.uX = 0.0f;
        person.uY = 0.0f;

        float seatX = -0.35f + seat.second * (seatWidth + 0.02f);
        float seatY = 0.17f - seat.first * (seatHeight + 0.08f);

        person.seatPosX = seatX + seatWidth / 2.0f;
        person.seatPosY = seatY + seatHeight;

        float doorX = doorDist(gen);
        person.doorPosX = doorX;
        person.doorPosY = 1.10f;

        person.texture = personTexture;
        people.push_back(person);
    }

    peopleInitialized = true;
}

void UpdatePeoplePositions(float deltaTime, float aspect) {
    CinemaState currentState = cinema.GetCinemaState();

    if (currentState != CinemaState::ENTERING &&
        currentState != CinemaState::LEAVING) {
        return;
    }

    for (auto& person : people) {
        if (person.state != PersonState::WALKING_TO_SEAT &&
            person.state != PersonState::WALKING_TO_EXIT) {
            continue;
        }

        if (person.state == PersonState::WALKING_TO_SEAT) {
            if (person.movingToY) {
                // Prvo po Y osi (vertikalno)
                float delta = person.speed * deltaTime;
                person.uY += delta;

                if (person.uY >= 1.0f) {
                    person.uY = 1.0f;
                    person.movingToY = false; // pređi na X kretanje
                }
            }
            else {
                // Onda po X osi (horizontalno)
                float delta = person.speed * deltaTime * aspect * 1.2;
                person.uX += delta;

                if (person.uX >= 1.0f) {
                    person.uX = 1.0f;
                    person.state = PersonState::SITTING; // sedi
                }
            }
        }
        else if (person.state == PersonState::WALKING_TO_EXIT) {
            if (!person.movingToY) {
                // Prvo po X osi (horizontalno)
                float delta = -person.speed * deltaTime * aspect * 1.2; // negativan smer
                person.uX += delta;

                if (person.uX <= 0.0f) {
                    person.uX = 0.0f;
                    person.movingToY = true; // pređi na Y kretanje
                }
            }
            else {
                // Onda po Y osi (vertikalno)
                float delta = -person.speed * deltaTime; // negativan smer
                person.uY += delta;

                if (person.uY <= 0.0f) {
                    person.uY = 0.0f;
                    person.state = PersonState::EXITED; // izašao
                }
            }
        }
    }
}

void SetupPersonForExit() {
    for (auto& person : people) {
        if (person.state == PersonState::SITTING) {
            person.state = PersonState::WALKING_TO_EXIT;
            person.movingToY = false;
        }
    }
}

void ResetPeople() {
    people.clear();
    peopleInitialized = false;
}

void formPersonVAO(float aspect) {
    float personWidth = 0.07f * aspect;
    float personHeight = 0.07f;

    float vertices[] = {
        -personWidth, -personHeight, 0.0f, 0.0f,
         personWidth, -personHeight, 1.0f, 0.0f,
         personWidth,  personHeight, 1.0f, 1.0f,
        -personWidth,  personHeight, 0.0f, 1.0f
    };

    glGenVertexArrays(1, &VAOperson);
    glGenBuffers(1, &VBOperson);

    glBindVertexArray(VAOperson);
    glBindBuffer(GL_ARRAY_BUFFER, VBOperson);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

void DrawPeople(unsigned personShader) {
    if (people.empty()) return;

    glUseProgram(personShader);
    glBindVertexArray(VAOperson);

    GLint texLoc = glGetUniformLocation(personShader, "uTexture");
    glUniform1i(texLoc, 0);
    glActiveTexture(GL_TEXTURE0);

    for (const auto& person : people) {
        if (person.state == PersonState::EXITED) continue;

        glBindTexture(GL_TEXTURE_2D, person.texture);

        GLint uXLoc = glGetUniformLocation(personShader, "uX");
        glUniform1f(uXLoc, person.uX);

        GLint uYLoc = glGetUniformLocation(personShader, "uY");
        glUniform1f(uYLoc, person.uY);

        GLint uDoorPosLoc = glGetUniformLocation(personShader, "uDoorPos");
        glUniform2f(uDoorPosLoc, person.doorPosX, person.doorPosY);

        GLint uSeatPosLoc = glGetUniformLocation(personShader, "uSeatPos");
        glUniform2f(uSeatPosLoc, person.seatPosX, person.seatPosY);

        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    }

    glBindVertexArray(0);
    glUseProgram(0);
}

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Formiranje prozora za prikaz sa datim dimenzijama i naslovom
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    screenWidth = mode->width;
    screenHeight = mode->height;
    // Uzimaju se širina i visina monitora
    // Moguće je igrati se sa aspect ratio-m dodavanjem frame buffer size callback-a
    GLFWwindow* window = glfwCreateWindow(screenWidth, screenHeight, "Bioskop", monitor, NULL);
    if (window == NULL) return endProgram("Prozor nije uspeo da se kreira.");
    glfwMakeContextCurrent(window);
    if (glewInit() != GLEW_OK) return endProgram("GLEW nije uspeo da se inicijalizuje.");

    // Postavi callback za klik mišem
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetKeyCallback(window, key_callback);

    cursor = loadImageToCursor("Resources/camera.png");
    glfwSetCursor(window, cursor);

    preprocessTexture(seatTextures[0], "Resources/available_seat.png");
    preprocessTexture(seatTextures[1], "Resources/reserved_seat.png");
    preprocessTexture(seatTextures[2], "Resources/reserved_sitting_seat.png");
    preprocessTexture(seatTextures[3], "Resources/bought_seat.png");
    preprocessTexture(seatTextures[4], "Resources/bought_sitting_seat.png");
    preprocessTexture(personTexture, "Resources/person.png");
    preprocessTexture(watermarkTexture, "Resources/watermark.png");

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glClearColor(0.31f, 0.31f, 0.31f, 1.0f);

    float aspect = (float)screenHeight / (float)screenWidth;

    unsigned int seatShader = createShader("seat.vert", "seat.frag");
    unsigned int screenShader = createShader("color.vert", "color.frag");
    unsigned int rectShader = createShader("rect.vert", "rect.frag");
    unsigned int personShader = createShader("person.vert", "person.frag");
    unsigned int watermarkShader = createShader("watermark.vert", "watermark.frag");

    float seatWidth = 0.1f * aspect;
    float seatHeight = 0.1f;

    std::vector<float> seats = GenerateSeatQuads(
        -0.35f,
        0.2f,
        0.1f,
        aspect,
        0.02f,
        0.08f
    );

    unsigned int VAOseats, VBOseats;

    formSeatsVAO(seats.data(), seats.size() * sizeof(float), VAOseats, VBOseats);

    unsigned int VAOscreen, VBOscreen;

    formScreenVAO(VAOscreen, VBOscreen);

    unsigned int VAOdoor, VBOdoor;

    formDoorVAO(VAOdoor, VBOdoor, aspect);

    formPersonVAO(aspect);

    unsigned int VAOdark, VBOdark;

    formDarkVAO(VAOdark, VBOdark);

    unsigned int VAOwatermark, VBOwatermark;

    formWatermarkVAO(VAOwatermark, VBOwatermark);

    bool hasOpenedDoor = false;

    while (!glfwWindowShouldClose(window))
    {
        double initFrameTime = glfwGetTime();

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            break;
        }

        glClear(GL_COLOR_BUFFER_BIT);

        drawSeats(seatShader, VAOseats);
        drawScreen(screenShader, VAOscreen);
        drawDoor(rectShader, VAOdoor);
        if (cinema.GetCinemaState() == CinemaState::ENTERING || cinema.GetCinemaState() == CinemaState::LEAVING) {
            DrawPeople(personShader);   
        }
        if (cinema.GetCinemaState() == CinemaState::SELLING) {
            drawDark(rectShader, VAOdark);
        }
        drawWatermark(watermarkShader, VAOwatermark);

        float deltaTime = 1.0f / 75.0f;

        if (enteringStart != NULL) {
            if (!peopleInitialized) {
                InitializePeople(seatWidth, seatHeight, aspect);
            }
            UpdatePeoplePositions(deltaTime, aspect);

            if (glfwGetTime() - enteringStart > 5) {
                enteringStart = NULL;
                cinema.SwitchState();
                formDoorVAO(VAOdoor, VBOdoor, aspect);
                hasOpenedDoor = false;
                cinema.SitOnSeats();
                playingStart = glfwGetTime();
            }
            else if (!hasOpenedDoor) {
                formDoorVAO(VAOdoor, VBOdoor, aspect);
                hasOpenedDoor = true;
            }
        }
        else if (playingStart != NULL) {
            if (glfwGetTime() - playingStart > 20) {
                playingStart = NULL;
                cinema.SwitchState();
                cinema.ResetFrameCounter();
                formDoorVAO(VAOdoor, VBOdoor, aspect);
                cinema.StandUp();
                leavingStart = glfwGetTime();

                SetupPersonForExit();
            }
            else {
                cinema.IncreaseFrameCounter();
            }
        }
        else if (leavingStart != NULL) {
            UpdatePeoplePositions(deltaTime, aspect);

            if (glfwGetTime() - leavingStart > 5) {
                leavingStart = NULL;
                cinema.SwitchState();
                formDoorVAO(VAOdoor, VBOdoor, aspect);
                cinema.ResetSeats();
                ResetPeople();
            }
        }

        glfwSwapBuffers(window);
        glfwPollEvents();

        while (glfwGetTime() - initFrameTime < 1 / 75.0) {}
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}