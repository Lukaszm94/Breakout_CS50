//
// breakout.c
//
// Computer Science 50
// Problem Set 3
//

// standard libraries
#define _XOPEN_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

// Stanford Portable Library
#include <spl/gevents.h>
#include <spl/gobjects.h>
#include <spl/gwindow.h>

// height and width of game's window in pixels
#define HEIGHT 600
#define WIDTH 400

// number of rows of bricks
#define ROWS 5

// number of columns of bricks
#define COLS 10

// radius of ball in pixels
#define RADIUS 10

// lives
#define LIVES 3

#define PADDLE_WIDTH 50.0
#define PADDLE_HEIGHT 10.0
#define PADDLE_POSITION_FROM_BOTTOM 50

#define BRICK_WIDTH 30
#define BRICK_HEIGHT 10
#define BRICKS_POSITION_FROM_TOP 30
#define BRICKS_INTERVAL_X 8
#define BRICKS_INTERVAL_Y 6

#define BALL_COLOR "BLACK"
#define BALL_STOPPED_MAX_SPEED 0.001

#define TIME_STEP 10

//bricks colors from highest to lowest row
char* brickColors[] = { "RED", "ORANGE", "YELLOW", "GREEN", "CYAN"};

// prototypes
void initBricks(GWindow window);
GOval initBall(GWindow window);
GRect initPaddle(GWindow window);
GLabel initScoreboard(GWindow window);
void updateScoreboard(GWindow window, GLabel label, int points);
GObject detectCollision(GWindow window, GOval ball);

void onMouseMoved(GRect paddle, double newPositionX);
void setPaddleCenterAtPosition(GRect paddle, double centerPosition);
void gameUpdate(GWindow window, GOval ball, GRect paddle, GLabel scoreLabel, int* score, int* lives, int* bricks);
void ballMovementUpdate(GWindow window, GOval ball);
void handleCollisions(GWindow window, GOval ball, GRect paddle, int* score, int* bricks);
void centerLabel(GWindow window, GLabel label);
bool isBallAtWindowBottom(GWindow window, GOval ball);
bool isBallStopped();
void printSummary(GWindow window, GLabel label, int score);
double getRandomBallSpeed();
void placeBallAtCentre(GWindow window, GOval ball);


double ballSpeedX = 0;
double ballSpeedY = 0;

int main(void)
{
    // seed pseudorandom number generator
    srand48(time(NULL));

    // instantiate window
    GWindow window = newGWindow(WIDTH, HEIGHT);

    // instantiate bricks
    initBricks(window);

    // instantiate ball, centered in middle of window
    GOval ball = initBall(window);

    // instantiate paddle, centered at bottom of window
    GRect paddle = initPaddle(window);

    // instantiate scoreboard, centered in middle of window, just above ball
    GLabel label = initScoreboard(window);

    // number of bricks initially
    int bricks = COLS * ROWS;

    // number of lives initially
    int lives = LIVES;

    // number of points initially
    int score = 0;
    

    // keep playing until game over
    while (lives > 0 && bricks > 0)
    {
        if(isBallStopped()) {
            waitForClick();
            ballSpeedX = getRandomBallSpeed();
            ballSpeedY = getRandomBallSpeed();
        }
        gameUpdate(window, ball, paddle, label, &score, &lives, &bricks);
        pause(TIME_STEP);
    }
    printSummary(window, label, score);

    // wait for click before exiting
    waitForClick();

    // game over
    closeGWindow(window);
    return 0;
}

void gameUpdate(GWindow window, GOval ball, GRect paddle, GLabel scoreLabel, int* score, int* lives, int* bricks)
{
    GEvent event = getNextEvent(MOUSE_EVENT);
    if (event != NULL)
    {
        // if the event was movement
        if (getEventType(event) == MOUSE_MOVED)
        {
            onMouseMoved(paddle, getX(event));
        }
    }
    ballMovementUpdate(window, ball);
    handleCollisions(window, ball, paddle, score, bricks);
    updateScoreboard(window, scoreLabel, *score);
    if(isBallAtWindowBottom(window, ball)) {
        ballSpeedX = 0.0;
        ballSpeedY = 0.0;
        placeBallAtCentre(window, ball);
        (*lives)--;
    }
}

bool isBallAtWindowBottom(GWindow window, GOval ball)
{
    double ballBottomY = getY(ball) + 2*RADIUS;
    bool result = (ballBottomY >= HEIGHT);
    return result;
}

bool isBallStopped()
{
    return ((fabs(ballSpeedX) < BALL_STOPPED_MAX_SPEED) && (fabs(ballSpeedY) < BALL_STOPPED_MAX_SPEED));
}

void printSummary(GWindow window, GLabel label, int score)
{
    const int summaryTabSize = 40;
    char summary[summaryTabSize];
    sprintf(summary, "GAME OVER\n Score: %i", score);
    setLabel(label, summary);
    centerLabel(window, label);
}

/*
 *Move ball and detect collisions with window borders. If collision detected, change speed in one axis
 *
 */

void ballMovementUpdate(GWindow window, GOval ball)
{
    //top left corner coordinates
    double newPositionX = getX(ball) + ballSpeedX * TIME_STEP;
    double newPositionY = getY(ball) + ballSpeedY * TIME_STEP;
    
    //collision with left or right side
    if((newPositionX) <= 0 || (newPositionX + 2 * RADIUS) >= WIDTH) {
        ballSpeedX = -ballSpeedX;
    }
    //collision with top is only detected
    if(newPositionY <= 0) {
        ballSpeedY = -ballSpeedY;
    }
    
    setLocation(ball, newPositionX, newPositionY);
}

void onMouseMoved(GRect paddle, double newPositionX)
{
    setPaddleCenterAtPosition(paddle, newPositionX);
}

void handleCollisions(GWindow window, GOval ball, GRect paddle, int* score, int* bricks)
{
    GObject touchedObject = detectCollision(window, ball);
    if(touchedObject == NULL) {
    return;
    }
    if(touchedObject == paddle) {
        if(ballSpeedY > 0) {
            ballSpeedY = -ballSpeedY;
        }
    }
    else if(strcmp(getType(touchedObject), "GRect") == 0) {
        printf("Collision with brick\n");
        removeGWindow(window, touchedObject);
        ballSpeedY = - ballSpeedY;
        (*score)++;
        (*bricks)--;
    }
}

void setPaddleCenterAtPosition(GRect paddle, double centerPosition)
{
    double newTopLeftX = centerPosition - PADDLE_WIDTH/2;
    if(newTopLeftX < 0) {
        newTopLeftX = 0;
    }
    else if((newTopLeftX + PADDLE_WIDTH) > WIDTH) {
        newTopLeftX = WIDTH - PADDLE_WIDTH;
    }
    setLocation(paddle, newTopLeftX, getY(paddle));
}

/**
 * Initializes window with a grid of bricks.
 */
void initBricks(GWindow window)
{
    double overallRowWidth = BRICK_WIDTH*COLS + BRICKS_INTERVAL_X*(COLS - 1);
    double firstLeftCornerX = (WIDTH - overallRowWidth)/2;
    if(firstLeftCornerX < 0) {
        firstLeftCornerX = 0;
    }
    for(int i = 0; i < ROWS; i++) {
        char* color = brickColors[i];
        double topLeftX = firstLeftCornerX;
        double topLeftY = BRICKS_POSITION_FROM_TOP + (BRICK_HEIGHT + BRICKS_INTERVAL_Y) * i;
        for(int i = 0; i < COLS; i++) {
            GRect brick = newGRect(topLeftX, topLeftY, BRICK_WIDTH, BRICK_HEIGHT);
            setFilled(brick, true);
            setColor(brick, color);
            add(window, brick);
            topLeftX += (BRICK_WIDTH + BRICKS_INTERVAL_X);
        }
    }
}

double getRandomBallSpeed()
{
    //double speed = drand48()/3;
    double speed = 0.3;
    return speed;
}

void placeBallAtCentre(GWindow window, GOval ball)
{
    double ballCenterX = WIDTH/2;
    double ballCenterY = HEIGHT/2;
    
    double topLeftX = ballCenterX - RADIUS;
    double topLeftY = ballCenterY - RADIUS;
    
    setLocation(ball, topLeftX, topLeftY);
}

/**
 * Instantiates ball in center of window.  Returns ball.
 */
GOval initBall(GWindow window)
{
    double ballCenterX = WIDTH/2;
    double ballCenterY = HEIGHT/2;
    
    double topLeftX = ballCenterX - RADIUS;
    double topLeftY = ballCenterY - RADIUS;
    
    GOval ball = newGOval(topLeftX, topLeftY, 2*RADIUS, 2*RADIUS);
    setFilled(ball, true);
    setColor(ball, BALL_COLOR);
    add(window, ball);
    
    return ball;
}

/**
 * Instantiates paddle in bottom-middle of window.
 */
GRect initPaddle(GWindow window)
{
    double paddleTopLeftCornerX = (WIDTH - PADDLE_WIDTH)/2;
    double paddleTopLeftCornerY = HEIGHT - PADDLE_HEIGHT - PADDLE_POSITION_FROM_BOTTOM;
    
    GRect paddle = newGRect(paddleTopLeftCornerX, paddleTopLeftCornerY, PADDLE_WIDTH, PADDLE_HEIGHT);
    setFillColor(paddle, "BLACK");
    setFilled(paddle, true); 
    add(window, paddle);
    return paddle;
}

/**
 * Instantiates, configures, and returns label for scoreboard.
 */
GLabel initScoreboard(GWindow window)
{
    GLabel label = newGLabel("0");
    setFont(label, "SansSerif-32");
    double x = (getWidth(window) - getWidth(label)) / 2;
    double y = (getHeight(window) - getHeight(label)) / 2;
    setLocation(label, x, y);
    add(window, label);
    return label;
}

void centerLabel(GWindow window, GLabel label)
{
    double x = (getWidth(window) - getWidth(label)) / 2;
    double y = (getHeight(window) - getHeight(label)) / 2;
    setLocation(label, x, y);
}

/**
 * Updates scoreboard's label, keeping it centered in window.
 */
void updateScoreboard(GWindow window, GLabel label, int points)
{
    // update label
    char s[12];
    sprintf(s, "%i", points);
    setLabel(label, s);

    // center label in window
    centerLabel(window, label);
}

/**
 * Detects whether ball has collided with some object in window
 * by checking the four corners of its bounding box (which are
 * outside the ball's GOval, and so the ball can't collide with
 * itself).  Returns object if so, else NULL.
 */
GObject detectCollision(GWindow window, GOval ball)
{
    // ball's location
    double x = getX(ball);
    double y = getY(ball);

    // for checking for collisions
    GObject object;

    // check for collision at ball's top-left corner
    object = getGObjectAt(window, x, y);
    if (object != NULL)
    {
        return object;
    }

    // check for collision at ball's top-right corner
    object = getGObjectAt(window, x + 2 * RADIUS, y);
    if (object != NULL)
    {
        return object;
    }

    // check for collision at ball's bottom-left corner
    object = getGObjectAt(window, x, y + 2 * RADIUS);
    if (object != NULL)
    {
        return object;
    }

    // check for collision at ball's bottom-right corner
    object = getGObjectAt(window, x + 2 * RADIUS, y + 2 * RADIUS);
    if (object != NULL)
    {
        return object;
    }

    // no collision
    return NULL;
}
