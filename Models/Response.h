#pragma once

enum class Response
{
    // Succesful
    OK,
    // Return move back in this game
    BACK,
    // Replay this game
    REPLAY,
    // Quit from this game
    QUIT,
    // Move in this game again
    CELL
};
