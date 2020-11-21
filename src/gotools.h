#ifndef GOTOOLS_H
#define GOTOOLS_H

extern go_board   new_handicap_board(int, int);
extern bit_array  calculate_hoshis(const go_board &);
extern board_rect find_crop(const game_state *gs);

#endif
