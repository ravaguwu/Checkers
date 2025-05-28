#pragma once

enum class Response
{
    // Успешно
    OK,
    // Вернуть ход
    BACK,
    // Переигровка
    REPLAY,
    // Выход
    QUIT,
    // Ход
    CELL
};
