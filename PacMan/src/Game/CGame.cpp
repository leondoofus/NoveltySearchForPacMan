#include <CGame.h>
#include <CStateManager.h>
#include <CGameLayout.h>
#include <CInputManager.h>
#include <CBoardUtils.h>
#include <CDialog.h>

#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <fstream>
#include <iostream>

#include <simulation.h>
#include "CGame.h"


#define MAX_INPUTS 13
#define MAX_OUTPUTS 1

#define NB_REENTRANT 0

#ifdef USE_NCURSES
#define DELTA_TIME 300000
#else
#define DELTA_TIME 3000
#endif
#define DELTA_STEP 100

//#define VERBOSE


static int counter_ghost = 0;
vector<float> CGame::path {-1};

template<typename T>
void safe_delete(T *&a) {
    delete a;
    a = NULL;
}


CGame::CGame() :
        game_over(true),
        paused(false),
        show_pause_menu(false),
        player(NULL),
        board(NULL),
        layout(NULL),
        ghost(NULL),
        pause_menu(NULL),
        bool_exit(false),
        network(NULL),
        last_action(-1),
        bool_menu(false),
        last_time_eat(0) {
    //load_high_score ();
}

CGame::CGame(std::string map, int difficulty) :
        game_over(true),
        paused(false),
        show_pause_menu(false),
        player(NULL),
        board(NULL),
        layout(NULL),
        ghost(NULL),
        pause_menu(NULL),
        bool_exit(false),
        bool_menu(false),
        map(map),
        difficulty(difficulty),
        stage(1),
        high_score(0),
        points(0),
        network(NULL),
        last_action(-1),
        cursor(0),
        last_time_eat(0) {
    //load_high_score ();
    }

CGame::CGame(std::string map, int difficulty, vector<int> params) :
        game_over(true),
        paused(false),
        show_pause_menu(false),
        player(NULL),
        board(NULL),
        layout(NULL),
        ghost(NULL),
        pause_menu(NULL),
        bool_exit(false),
        bool_menu(false),
        map(map),
        difficulty(difficulty),
        stage(1),
        high_score(0),
        points(0),
        params(params),
        network(NULL),
        last_action(-1),
        cursor(0),
        last_time_eat(0) {
    //load_high_score ();
}

CGame::CGame(std::string map, int difficulty, Network *network) :
        game_over(true),
        paused(false),
        show_pause_menu(false),
        player(NULL),
        board(NULL),
        layout(NULL),
        ghost(NULL),
        pause_menu(NULL),
        bool_exit(false),
        bool_menu(false),
        map(map),
        difficulty(difficulty),
        stage(1),
        high_score(0),
        points(0),
        cursor(0),
        network(network),
        last_action(-1),
        last_time_eat(0) {
    //load_high_score ();
}

CGame::~CGame() {
    safe_delete(board);
    safe_delete(player);
    safe_delete(layout);
    safe_delete(ghost);
    safe_delete(pause_menu);

}

void CGame::start() {
    safe_delete(board);
    safe_delete(player);
    safe_delete(layout);
    safe_delete(ghost);
    safe_delete(pause_menu);


    //if ( high_score >= points)
    //		save_high_score ();

    if (game_over) {
        points = 0;
        lives = 0;
        //lives  = 3;
        stage = 1;
    }
    //load_high_score ();

    game_over = false;
    show_pause_menu = false;
    paused = false;


    if (!map.empty())
        board = CBoardUtils::load_file(map);
    else
        board = CBoardUtils::load_file("default");

    player = new CPlayer(board->get_start_pos());

    ghost = new CGhostManager(board->get_ghost_x(),
                              board->get_ghost_y(),
                              difficulty, points);

#ifdef USE_NCURSES
    int cur_h, cur_w;
    getmaxyx ( stdscr, cur_h, cur_w );

    if ( cur_h - 2 < board -> get_height () ||
         cur_w - 2 < board -> get_width () )
        {
            std::vector<std::string> v = {  "Map is bigger than your actual",
            "window. Please resize it ", "and run game again" };
            CDialog::show ( v, "ERROR" );
            bool_exit = true;
        }

    layout = new CGameLayout (this, cur_w, cur_h);

    pause_menu = new CMenu ( 1, 1, layout -> pause_menu -> get_width () - 2,
                             layout -> pause_menu -> get_height () - 2 );

    CMenuItem * item;

    item = new CMenuItem ( "Resume Game", RESUME);
    pause_menu -> add ( item );

    item = new CMenuItem ( "Restart Game", RESTART);
    pause_menu -> add ( item );

    pause_menu -> add ( NULL );

    item = new CMenuItem ( "Main Menu", MAIN_MENU);
    pause_menu -> add ( item );

    item = new CMenuItem ( "Exit Game", EXIT_GAME);
    pause_menu -> add ( item );
#endif

    timer_player.start();
    timer_ghost.start();
    timer.start();

    counter_ghost = 0;
    CGame::path = {-1};
}

void CGame::restart() {
    safe_delete(player);
    safe_delete(layout);
    safe_delete(ghost);

    player = new CPlayer(board->get_start_pos());
    ghost = new CGhostManager(32, 9, difficulty, points);

#ifdef USE_NCURSES
    int cur_h, cur_w;
    getmaxyx ( stdscr, cur_h, cur_w );

    layout = new CGameLayout (this, cur_w, cur_h);
#endif

    timer_player.start();
    timer_ghost.start();
    timer.start();

    counter_ghost = 0;
    CGame::path = {-1};

}

bool CGame::is_paused() const {
    return paused;
}

bool CGame::is_over() const {
    return game_over;
}

bool CGame::will_exit() const {
    return bool_exit;
}

bool CGame::will_go_to_menu() const {
    return bool_menu;
}

void CGame::handle_input() {

    //if (cursor == 0 && params.size() == 0){
    int r = 0;
    if (!network) {
        std::cout << "Warning : no network ! Picking random action." << std::endl;
        r = rand() % 5;
        if (r == 0) {
            last_action = 0;
        } else if (r == 1) {
            last_action = 1;
            this->player->move(CPlayer::LEFT);
        } else if (r == 2) {
            last_action = 2;
            this->player->move(CPlayer::RIGHT);
        } else if (r == 3) {
            last_action = 3;
            this->player->move(CPlayer::UP);
        } else {
            last_action = 4;
            this->player->move(CPlayer::DOWN);
        }
    } else {

        double inputs[MAX_INPUTS];
        inputs[0] = 1.0;
        inputs[1] = (double) get_sensor_left() / 10.0;
        inputs[2] = (double) get_sensor_right() / 10.0;
        inputs[3] = (double) get_sensor_up() / 10.0;
        inputs[4] = (double) get_sensor_down() / 10.0;

        inputs[5] = 1.0/(double) get_ghost_left();
        inputs[6] = 1.0/(double) get_ghost_right();
        inputs[7] = 1.0/(double) get_ghost_up();
        inputs[8] = 1.0/(double) get_ghost_down();

        inputs[9]  = (double) get_dots_left() / 10.0;
        inputs[10] = (double) get_dots_right() / 10.0;
        inputs[11] = (double) get_dots_up() / 10.0;
        inputs[12] = (double) get_dots_down() / 10.0;

        for (int i = 1; i < MAX_INPUTS; i++) {
            if(inputs[i] > 1.0) inputs[i] = 1.0;
            if(inputs[i] < 0.0) inputs[i] = 0.0;
        }


        network->load_sensors(inputs);
        network->activate();
        auto outputs = network->outputs;

/*
        for (int i = 5; i < 5 + NB_REENTRANT; i++) {
            reentrant_nodes[i] = outputs[i]->get_active_out();
        }
*/

        //output
        r = (int) (outputs[0]->get_active_out() * 5);

    }
#ifdef VERBOSE
    std::cout << "Picked choice : " << r << std::endl;
#endif
    if (r == 0) {
        last_action = 0;
    } else if (r == 1) {
        last_action = 1;
        this->player->move(CPlayer::LEFT);
    } else if (r == 2) {
        last_action = 2;
        this->player->move(CPlayer::RIGHT);
    } else if (r == 3) {
        last_action = 3;
        this->player->move(CPlayer::UP);
    } else {
        last_action = 4;
        this->player->move(CPlayer::DOWN);
    }




//        if (cursor > params.size()) {
//            return;
//        } else {
//            if (params[cursor] == 1) this->player->move(CPlayer::RIGHT);
//            else if (params[cursor] == 2) this->player->move(CPlayer::LEFT);
//            else if (params[cursor] == 3) this->player->move(CPlayer::UP);
//            else if (params[cursor] == 4)this->player->move(CPlayer::DOWN);
//            ++cursor;
//            return;
//        }


    /*if ( ! CInputManager::any_key_pressed () )
        return;

    if ( CInputManager::is_pressed ( 'p' ))
        {
            paused = ( is_paused () ) ? false : true;
            return;
        }
    if ( is_paused () )
        {
            pause_menu -> HandleInput ();
            return;
        }
    if ( CInputManager::is_pressed (	KEY_RIGHT  ))
        this -> player -> move ( CPlayer::RIGHT );
    if ( CInputManager::is_pressed (	KEY_LEFT  ))
        this -> player -> move ( CPlayer::LEFT );
    if ( CInputManager::is_pressed (	KEY_DOWN  ))
        this -> player -> move ( CPlayer::DOWN );
    if ( CInputManager::is_pressed (	KEY_UP  ))
        this -> player -> move ( CPlayer::UP );*/

}

bool CGame::is_food_eaten() {
    if (board->is_dot(player->getX(), player->getY())) {
        board->eat_dot(player->getX(), player->getY());
        points += 10;
        last_time_eat = 0;
        return true;
    }
    last_time_eat ++;
    return false;
}

bool CGame::is_star_eaten() {
    if (board->is_star(player->getX(), player->getY())) {
        board->eat_dot(player->getX(), player->getY());
        points += 50;
        ghost->set_frightened_mode(true);
        last_time_eat = 0;
        return true;
    }
    //last_time_eat ++; //already in function above
    return false;
}

void CGame::update() {
    // Stop 
    if (timer.d_ms() >= DELTA_TIME || last_time_eat >= DELTA_STEP) CStateManager::quitScore(points);

    timer_player.pause();
    timer_ghost.pause();

    int delta = this->get_delay(stage);
    int delta_ghost;

    if (points > high_score)
        high_score = points;

    //TODO replace this with victory event
    if (board->all_dots_eaten()) { //Dont't know if we should keep this
        std::vector<std::string> v = {"Your score is " + points};
        CDialog::show(v, "LEVEL PASSED");
        stage++;
        start();
    }

    if (is_paused()) {
        if (pause_menu->WillExit()) {
            int option = pause_menu->CurrentID();

            switch (option) {
                case RESUME:
                    paused = false;
                    break;
                case RESTART:
                    game_over = true;
                    start();
                    return;
                case MAIN_MENU:
                    bool_menu = true;
                    return;
                case EXIT_GAME:
                    bool_exit = true;
                    return;
            }
            pause_menu->reset();
        }

        return;
    }

 #ifdef USE_NCURSES
    if (timer_player.d_ms() >= delta) {
 #else
     if (true) {
 #endif
        if (!player->is_alive()) {
            if (lives > 0) {
                lives--;
                std::vector<std::string> v = {"You have " +
                                              std::to_string(lives) +
                                              " lives left."};
                CDialog::show(v, "You Died");
                restart();

            } else {
                /*game_over = true;
                std::vector<std::string> v = {  "Your score is: " +
                                                 std::to_string(points) + "." };
                if ( high_score <= points )
                    v . push_back ( "NEW HIGHSCORE!!!");
                CDialog::show ( v , "GAME OVER");*/
                //pause_menu -> RemoveByID ( RESUME );

                /*std::ofstream score;
                score . open("./examples/scores/" + map + ".pacscore", std::ios::app);
                score << points << std::endl;
                score . close();*/
                CStateManager::quitScore(points);
                //CNCurses::exit();
                //exit(0);
                /*paused = true;
                pause_menu -> RemoveByID ( RESUME );*/
            }
        } else {
            points--; //remove points for fitness calculation -> the faster, the better
            player->update(board);
            ghost->check_collisions(player);
            is_food_eaten();
            is_star_eaten();
            update_path();
        }

        timer_player.start();
    } else {
        timer_player.unpause();
    }
    if (ghost->are_frightened())
        //delta_ghost = 1 + delta;
         delta_ghost = 1.6 * delta;
    else delta_ghost = delta;

#ifdef USE_NCURSES
    if ( timer_ghost . d_ms () >= delta_ghost )
#else
    if (!(counter_ghost % 3) || !ghost->are_frightened())  //slow down ghosts in training mode
#endif
    {
        ghost->update(board, player);
        ghost->check_collisions(player);
        timer_ghost.start();
    } else timer_ghost.unpause();

    if ( ghost -> are_frightened () )
            delta_ghost = 1.6 * delta;
        else delta_ghost = delta;

        if ( timer_ghost . d_ms () >= delta_ghost )
            {
                ghost ->  update ( board, player );
                ghost -> check_collisions ( player );
                timer_ghost . start ();
            }
        else timer_ghost . unpause ();
}

void CGame::draw() {
    layout->draw(pause_menu);
}

int CGame::get_delay(int speed) const {
     switch (speed) {
         case 1:
             return 120;
         case 2:
             return 100;
         case 3:
             return 90;
         case 4:
             return 80;
         case 5:
             return 60;
         default:
             return 30;
     }
}

void CGame::load_high_score() {
    std::ifstream score("./examples/scores/" + map + ".pacscore");
    if (score.good())
        score >> high_score;
    else high_score = 0;
}

void CGame::save_high_score() {
    std::ofstream score;
    score.open("./examples/scores/" + map + ".pacscore", std::ofstream::out | std::ofstream::trunc);
    score << high_score;
    score.close();
}

int CGame::get_sensor_left() {
    int x = player->getX();
    int y = player->getY();
    int val = 0;
    int i = x;
    while (i > 0) {
        if (!board->is_wall(i, y) && !board->is_border(i, y) && !ghost->is_ghost(i, y)) {
            val++;
            i--;
        } else return val;
    }
    return val;
}

int CGame::get_sensor_right() {
    int x = player->getX();
    int y = player->getY();
    int val = 0;
    int i = x;
    while (i < board->get_width()) {
        if (!board->is_wall(i, y) && !board->is_border(i, y) && !ghost->is_ghost(i, y)) {
            val++;
            i++;
        } else return val;
    }
    return val;
}

int CGame::get_sensor_up() {
    int x = player->getX();
    int y = player->getY();
    int val = 0;
    int i = y;
    while (i > 0) {
        if (!board->is_wall(x, i) && !board->is_border(x, i) && !ghost->is_ghost(x, i)) {
            val++;
            i--;
        } else return val;
    }
    return val;
}

int CGame::get_sensor_down() {
    int x = player->getX();
    int y = player->getY();
    int val = 0;
    int i = y;
    while (i > board->get_height()) {
        if (!board->is_wall(x, i) && !board->is_border(x, i) && !ghost->is_ghost(x, i)) {
            val++;
            i++;
        } else return val;
    }
    return val;
}

int CGame::get_ghost_left()
{
    int x = player->getX();
    int y = player->getY();
    int val = 0;
    int i = x;
    while (i > 0)
    {
        if (board->is_wall(i, y) || board->is_border(i, y)) return -1;
        else if (!ghost->is_ghost(i,y)) val++;
        else return val+1;
        i--;
    }
    return -1;
}
int CGame::get_ghost_right()
{
    int x = player->getX();
    int y = player->getY();
    int val = 0;
    int i = x;
    while (i < board->get_width())
    {
        if (board->is_wall(i, y) || board->is_border(i, y)) return -1;
        else if (!ghost->is_ghost(i,y)) val++;
        else return val+1;
        i++;
    }
    return -1;
}
int CGame::get_ghost_up()
{
    int x = player->getX();
    int y = player->getY();
    int val = 0;
    int i = y;
    while (i > 0)
    {
        if (board->is_wall(x, i) || board->is_border(x, i)) return -1;
        else if (!ghost->is_ghost(x,i)) val++;
        else return val+1;
        i--;
    }
    return -1;
}
int CGame::get_ghost_down()
{
    int x = player->getX();
    int y = player->getY();
    int val = 0;
    int i = y;
    while (i < board->get_height())
    {
        if (board->is_wall(x, i) || board->is_border(x, i)) return -1;
        else if (!ghost->is_ghost(x,i)) val++;
        else return val+1;
        i++;
    }
    return -1;
}

int CGame::get_dots_left()
{
    int x = player->getX();
    int y = player->getY();
    int val = 0;
    int i = x;
    while (i > 0)
    {
        if (board->is_wall(i, y) || board->is_border(i, y) || ghost->is_ghost(i,y)) return val;
        else if (!board->is_empty_spot(i,y)) val++;
        i--;
    }
    return val;
}
int CGame::get_dots_right()
{
    int x = player->getX();
    int y = player->getY();
    int val = 0;
    int i = x;
    while (i < board->get_width())
    {
        if (board->is_wall(i, y) || board->is_border(i, y) || ghost->is_ghost(i,y)) return val;
        else if (!board->is_empty_spot(i,y)) val++;
        i++;
    }
    return val;
}
int CGame::get_dots_up()
{
    int x = player->getX();
    int y = player->getY();
    int val = 0;
    int i = y;
    while (i > 0)
    {
        if (board->is_wall(x, i) || board->is_border(x, i) || ghost->is_ghost(x,i)) return val;
        else if (!board->is_empty_spot(x,i)) val++;
        i--;
    }
    return val;
}
int CGame::get_dots_down()
{
    int x = player->getX();
    int y = player->getY();
    int val = 0;
    int i = y;
    while (i < board->get_height())
    {
        if (board->is_wall(x, i) || board->is_border(x, i) || ghost->is_ghost(x,i)) return val;
        else if (!board->is_empty_spot(x,i)) val++;
        i++;
    }
    return val;
}

//transform map coordinates into a single value
int CGame::shrinkCoord(int x, int y) {
    return (x * board->get_width() + y) ;
}

bool CGame::isCorner(int x, int y) {
    return ((!board->is_wall(x, y - 1) && !board->is_border(x, y - 1))
            || (!board->is_wall(x, y + 1) && !board->is_border(x, y + 1))) &&
           ((!board->is_wall(x-1, y) && !board->is_border(x-1, y))
           || (!board->is_wall(x+1, y) && !board->is_border(x+1, y)));
}

//add current position to path if it is not already at the end
void CGame::update_path() {
    int x = player->getX();
    int y = player->getY();
    int c = shrinkCoord(x, y);
    if(isCorner(x, y) && *(CGame::path.end() - 1) != c){
        CGame::path.push_back(c);
    }
}
