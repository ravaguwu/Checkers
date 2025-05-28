#pragma once
#include <random>
#include <vector>

#include "../Models/Move.h"
#include "Board.h"
#include "Config.h"

const int INF = 1e9;

class Logic
{
  public:
    Logic(Board *board, Config *config) : board(board), config(config)
    {
        rand_eng = std::default_random_engine (
            !((*config)("Bot", "NoRandom")) ? unsigned(time(0)) : 0);
        scoring_mode = (*config)("Bot", "BotScoringType");
        optimization = (*config)("Bot", "Optimization");
    }

    vector<move_pos> find_best_turns(const bool color)
    {
        // Clear previous ones
        next_best_state.clear();
        next_move.clear();

        // Finding the best moves
        find_first_best_turn(board->get_board(), color, -1, -1, 0);

        // Finding the best sequence of moves
        vector<move_pos> res;
        int current_index = 0;
        while (current_index != -1 && next_move[current_index].x != -1)
        {
            res.push_back(next_move[current_index]);
            current_index = next_best_state[current_index];
        }
        return res;
    }

private:
    // We make a move at the transferred positions of checkers
    vector<vector<POS_T>> make_turn(vector<vector<POS_T>> mtx, move_pos turn) const
    {
        if (turn.xb != -1)
            mtx[turn.xb][turn.yb] = 0;
        if ((mtx[turn.x][turn.y] == 1 && turn.x2 == 0) || (mtx[turn.x][turn.y] == 2 && turn.x2 == 7))
            mtx[turn.x][turn.y] += 2;
        mtx[turn.x2][turn.y2] = mtx[turn.x][turn.y];
        mtx[turn.x][turn.y] = 0;
        return mtx;
    }

    // We count the score of the given checker positions
    double calc_score(const vector<vector<POS_T>> &mtx, const bool first_bot_color) const
    {
        // color - who is max player
        double w = 0, wq = 0, b = 0, bq = 0;
        for (POS_T i = 0; i < 8; ++i)
        {
            for (POS_T j = 0; j < 8; ++j)
            {
                w += (mtx[i][j] == 1);
                wq += (mtx[i][j] == 3);
                b += (mtx[i][j] == 2);
                bq += (mtx[i][j] == 4);
                if (scoring_mode == "NumberAndPotential")
                {
                    w += 0.05 * (mtx[i][j] == 1) * (7 - i);
                    b += 0.05 * (mtx[i][j] == 2) * (i);
                }
            }
        }
        if (!first_bot_color)
        {
            swap(b, w);
            swap(bq, wq);
        }
        if (w + wq == 0)
            return INF;
        if (b + bq == 0)
            return 0;
        int q_coef = 4;
        if (scoring_mode == "NumberAndPotential")
        {
            q_coef = 5;
        }
        return (b + bq * q_coef) / (w + wq * q_coef);
    }

    double find_first_best_turn(vector<vector<POS_T>> mtx, const bool color, const POS_T x, const POS_T y, size_t state,
        double alpha = -1)
    {
        // Re init for current state
        next_best_state.push_back(-1);
        next_move.emplace_back(-1, -1, -1, -1);
        double best_score = -1;

        // For current checker or for all
        if (state == 0) {
            find_turns(color, mtx);
        }
        else {
            find_turns(x, y, mtx);
        }

        // Save already finded turns
        auto turns_now = turns;
        bool have_beats_now = have_beats;

        // Go deepter if it not combo
        if (!have_beats_now && state != 0)
            return find_best_turns_rec(mtx, 1 - color, 0, alpha);

        // Finding the best one move in all possible
        for (auto turn : turns_now)
        {
            size_t next_state = next_move.size();
            double score;

            if (have_beats_now)
                score = find_first_best_turn(make_turn(mtx, turn), color, turn.x2, turn.y2, next_state, best_score);
            else
                score = find_best_turns_rec(make_turn(mtx, turn), 1 - color, 0, best_score);

            if (score > best_score)
            {
                best_score = score;
                next_best_state[state] = (have_beats_now ? static_cast<int>(next_state) : -1);
                next_move[state] = turn;
            }
        }

        return best_score;
    }

    double find_best_turns_rec(vector<vector<POS_T>> mtx, const bool color, const size_t depth, double alpha = -1,
        double beta = INF + 1, const POS_T x = -1, const POS_T y = -1)
    {
        // If max depth, we will calcute total score
        if (depth == Max_depth)
        {
            return calc_score(mtx, (depth % 2 == color));
        }

        // For only one checker or for all
        if (x != -1)
        {
            find_turns(x, y, mtx);
        }
        else
            find_turns(color, mtx);

        // Save already finded turns
        auto turns_now = turns;
        bool have_beats_now = have_beats;

        // Go deeper if it not combo
        if (!have_beats_now && x != -1)
        {
            return find_best_turns_rec(mtx, 1 - color, depth + 1, alpha, beta);
        }

        //If empty, return calcutiong
        if (turns.empty())
            return (depth % 2 == color % 2) ? 0 : INF;

        // Min scores
        double min_score = INF + 1;
        double max_score = -1;
        for (auto turn : turns_now)
        {
            // Score of current move
            double score;

            // Go deeper if it not combo
            if (!have_beats_now && x == -1)
            {
                score = find_best_turns_rec(make_turn(mtx, turn), 1 - color, depth + 1, alpha, beta);
            }
            else
            {
                // Otherwise current
                score = find_best_turns_rec(make_turn(mtx, turn), color, depth, alpha, beta, turn.x2, turn.y2);
            }

            min_score = min(min_score, score);
            max_score = max(max_score, score);

            if (depth % 2) // Max move
                alpha = max(alpha, max_score);
            else // Min move
                beta = min(beta, min_score);

            // If optimization we will cut
            if (optimization != "O0" && alpha >= beta)
                return (depth % 2) ? beta : alpha;
        }

        return (depth % 2) ? max_score : min_score;
    }

public:
    // Find possible moves for a certain color
    void find_turns(const bool color)
    {
        find_turns(color, board->get_board());
    }

    // We find possible moves for a checker in position
    void find_turns(const POS_T x, const POS_T y)
    {
        find_turns(x, y, board->get_board());
    }

private:
    // We find possible moves for a certain color, for the given position of checkers
    void find_turns(const bool color, const vector<vector<POS_T>> &mtx)
    {
        vector<move_pos> res_turns;
        bool have_beats_before = false;
        for (POS_T i = 0; i < 8; ++i)
        {
            for (POS_T j = 0; j < 8; ++j)
            {
                if (mtx[i][j] && mtx[i][j] % 2 != color)
                {
                    find_turns(i, j, mtx);
                    if (have_beats && !have_beats_before)
                    {
                        have_beats_before = true;
                        res_turns.clear();
                    }
                    if ((have_beats_before && have_beats) || !have_beats_before)
                    {
                        res_turns.insert(res_turns.end(), turns.begin(), turns.end());
                    }
                }
            }
        }
        turns = res_turns;
        shuffle(turns.begin(), turns.end(), rand_eng);
        have_beats = have_beats_before;
    }

    // We find possible moves for a certain color for the transferred position of checkers, for a checker in position
    void find_turns(const POS_T x, const POS_T y, const vector<vector<POS_T>> &mtx)
    {
        turns.clear();
        have_beats = false;
        POS_T type = mtx[x][y];
        // check beats
        switch (type)
        {
        case 1:
        case 2:
            // check pieces
            for (POS_T i = x - 2; i <= x + 2; i += 4)
            {
                for (POS_T j = y - 2; j <= y + 2; j += 4)
                {
                    if (i < 0 || i > 7 || j < 0 || j > 7)
                        continue;
                    POS_T xb = (x + i) / 2, yb = (y + j) / 2;
                    if (mtx[i][j] || !mtx[xb][yb] || mtx[xb][yb] % 2 == type % 2)
                        continue;
                    turns.emplace_back(x, y, i, j, xb, yb);
                }
            }
            break;
        default:
            // check queens
            for (POS_T i = -1; i <= 1; i += 2)
            {
                for (POS_T j = -1; j <= 1; j += 2)
                {
                    POS_T xb = -1, yb = -1;
                    for (POS_T i2 = x + i, j2 = y + j; i2 != 8 && j2 != 8 && i2 != -1 && j2 != -1; i2 += i, j2 += j)
                    {
                        if (mtx[i2][j2])
                        {
                            if (mtx[i2][j2] % 2 == type % 2 || (mtx[i2][j2] % 2 != type % 2 && xb != -1))
                            {
                                break;
                            }
                            xb = i2;
                            yb = j2;
                        }
                        if (xb != -1 && xb != i2)
                        {
                            turns.emplace_back(x, y, i2, j2, xb, yb);
                        }
                    }
                }
            }
            break;
        }
        // check other turns
        if (!turns.empty())
        {
            have_beats = true;
            return;
        }
        switch (type)
        {
        case 1:
        case 2:
            // check pieces
            {
                POS_T i = ((type % 2) ? x - 1 : x + 1);
                for (POS_T j = y - 1; j <= y + 1; j += 2)
                {
                    if (i < 0 || i > 7 || j < 0 || j > 7 || mtx[i][j])
                        continue;
                    turns.emplace_back(x, y, i, j);
                }
                break;
            }
        default:
            // check queens
            for (POS_T i = -1; i <= 1; i += 2)
            {
                for (POS_T j = -1; j <= 1; j += 2)
                {
                    for (POS_T i2 = x + i, j2 = y + j; i2 != 8 && j2 != 8 && i2 != -1 && j2 != -1; i2 += i, j2 += j)
                    {
                        if (mtx[i2][j2])
                            break;
                        turns.emplace_back(x, y, i2, j2);
                    }
                }
            }
            break;
        }
    }

  public:
    // Turns poses
    vector<move_pos> turns;
    // Are there beats
    bool have_beats;
    // Max depth of calculating
    int Max_depth;

  private:
    // Random number generator engine
    default_random_engine rand_eng;
    // Scoring mode
    string scoring_mode;
    // Optimization type
    string optimization;
    // Next moves poses
    vector<move_pos> next_move;
    // Next best states
    vector<int> next_best_state;
    // Board
    Board *board;
    // Config
    Config *config;
};
