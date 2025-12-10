#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>

#include "../Header/Util.h"
#include "../Cinema.hpp"

// Main fajl funkcija sa osnovnim komponentama OpenGL programa

// Projekat je dozvoljeno pisati počevši od ovog kostura
// Toplo se preporučuje razdvajanje koda po fajlovima (i eventualno potfolderima) !!!
// Srećan rad!

int screenWidth = 800;
int screenHeight = 800;
Cinema cinema = Cinema();

double enteringStart = NULL;
double playingStart = NULL;
double leavingStart = NULL;

/*
unsigned availableTexture;
unsigned reservedTexture;
unsigned boughtTexture;
unsigned reservedSittingTexture;
unsigned boughtSittingTexture; */

unsigned seatTextures[5];

GLFWcursor* cursor;

#include <vector>

struct SeatBounds {
    float x1, y1; // donji levi ugao
    float x2, y2; // gornji desni ugao
};

std::vector<SeatBounds> seatBounds; // Čuva granice svih sedišta

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
    data.reserve(ROWS * COLS * 16); // 4 temena × (x,y,s,t) = 16

    // Očistimo postojeće granice
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

            // push 4 verteksa
            data.insert(data.end(), {
                x1, y1, 0.0f, 1.0f,
                x2, y2, 1.0f, 1.0f,
                x3, y3, 1.0f, 0.0f,
                x4, y4, 0.0f, 0.0f
                });

            // Sačuvaj granice sedišta za detekciju klika
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

// Callback funkcija za klik mišem
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

// Callback funkcija za tastaturu
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    // Brojevi 1-9
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

    // ESC za izlaz
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }

    if (key == GLFW_KEY_ENTER && action == GLFW_PRESS) {
        if (enteringStart == NULL && playingStart == NULL && leavingStart == NULL) {
            enteringStart = glfwGetTime();
            cinema.SwitchState();
            cinema.GetRandomTakenSeats();
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

void drawDoor(unsigned shader, unsigned int VAOdoor)
{
    glUseProgram(shader);
    glBindVertexArray(VAOdoor);

    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

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

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glClearColor(0.31f, 0.31f, 0.31f, 1.0f);

    float aspect = (float)screenHeight / (float)screenWidth;

    unsigned int seatShader = createShader("seat.vert", "seat.frag");
    unsigned int screenShader = createShader("color.vert", "color.frag");
    unsigned int rectShader = createShader("rect.vert", "rect.frag");

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

        if (enteringStart != NULL) {
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
            }
            else {
                cinema.IncreaseFrameCounter();
            }
        }
        else if (leavingStart != NULL) {
            if (glfwGetTime() - leavingStart > 5) {
                leavingStart = NULL;
                cinema.SwitchState();
                formDoorVAO(VAOdoor, VBOdoor, aspect);
                cinema.ResetSeats();
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