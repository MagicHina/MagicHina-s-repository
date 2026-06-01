#include <graphics.h>
#include <string>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <tchar.h>
#include <windows.h>

const int WINDOW_WIDTH = 1280;
const int WINDOW_HEIGHT = 720;
const int PLAYER_ANIM_NUM = 6;

IMAGE img_player_left[PLAYER_ANIM_NUM];
IMAGE img_player_right[PLAYER_ANIM_NUM];
IMAGE img_background;
IMAGE img_shadow;

POINT player_pos = { 500, 500 };

const int BUTTON_WIDTH = 192;
const int BUTTON_HEIGHT = 75;

#pragma comment(lib,"Winmm.lib")
#pragma comment(lib,"MSIMG32.LIB")
bool running = true;
bool is_game_started = false;

inline void putimage_alpha(int x, int y, IMAGE* img)
{
    int w = img->getwidth();
    int h = img->getheight();
    AlphaBlend(GetImageHDC(NULL), x, y, w, h,
        GetImageHDC(img), 0, 0, w, h, { AC_SRC_OVER,0,255,AC_SRC_ALPHA });
}

class Atlas
{
public:
    std::vector<IMAGE*> frame_list;
    Atlas(LPCTSTR path, int num)
    {
        TCHAR path_file[256];
        for (size_t i = 0; i < num; i++)
        {
            _stprintf_s(path_file, 256, path, (int)i);
            IMAGE* frame = new IMAGE();
            loadimage(frame, path_file);
            frame_list.push_back(frame);
        }
    }
    ~Atlas()
    {
        for (size_t i = 0; i < frame_list.size(); i++)
        {
            delete frame_list[i];
        }
    }
};

Atlas* atlas_player_left;
Atlas* atlas_player_right;
Atlas* atlas_enemy_left;
Atlas* atlas_enemy_right;

enum class Status
{
    Idle,
    Hovered,
    Pushed
};

class Button
{
private:
    RECT region;
    IMAGE img_idle;
    IMAGE img_hovered;
    IMAGE img_pushed;
    bool CheckCursorHit(int x, int y)
    {
        return x >= region.left && x <= region.right && y >= region.top && y <= region.bottom;
    }
protected:
    virtual void OnClick() = 0;
    Status status = Status::Idle;
public:
    Button(RECT rect, LPCTSTR path_img_idle, LPCTSTR path_img_hovered, LPCTSTR path_img_pushed)
    {
        region = rect;
        loadimage(&img_idle, path_img_idle);
        loadimage(&img_hovered, path_img_hovered);
        loadimage(&img_pushed, path_img_pushed);
    }
    ~Button() = default;

    void Draw()
    {
        switch (status)
        {
        case Status::Idle:
            putimage(region.left, region.top, &img_idle);
            break;
        case Status::Hovered:
            putimage(region.left, region.top, &img_hovered);
            break;
        case Status::Pushed:
            putimage(region.left, region.top, &img_pushed);
            break;
        }
    }
    void ProcessEvent(const ExMessage& msg)
    {
        switch (msg.message)
        {
        case WM_MOUSEMOVE:
            if (status == Status::Idle && CheckCursorHit(msg.x, msg.y))
            {
                status = Status::Hovered;
            }
            else if (status == Status::Hovered && !CheckCursorHit(msg.x, msg.y))
            {
                status = Status::Idle;
            }
            break;
        case WM_LBUTTONDOWN:
            if (CheckCursorHit(msg.x, msg.y))
            {
                status = Status::Pushed;
            }
            break;
        case WM_LBUTTONUP:
            if (status == Status::Pushed)
            {
                OnClick();
            }
            break;
        default:
            break;
        }
    }
};

class StartGameButton :public Button
{
protected:
    void OnClick()
    {
        is_game_started = true;
        mciSendString(_T("play bgm repeat from 0"), NULL, 0, NULL);
    }
public:
    StartGameButton(RECT rect, LPCTSTR path_img_idle, LPCTSTR path_img_hovered, LPCTSTR path_img_pushed)
        :Button(rect, path_img_idle, path_img_hovered, path_img_pushed) {
    }
    ~StartGameButton() = default;
};

class QuitGameButton :public Button
{
protected:
    void OnClick()
    {
        running = false;
    }
public:
    QuitGameButton(RECT rect, LPCTSTR path_img_idle, LPCTSTR path_img_hovered, LPCTSTR path_img_pushed)
        :Button(rect, path_img_idle, path_img_hovered, path_img_pushed) {
    }
    ~QuitGameButton() = default;
};

class Animation
{
private:
    int timer = 0;
    int idx_frame = 0;
    int interval_ms = 0;
    Atlas* anim_atlas;
public:
    Animation(Atlas* atlas, int interval)
    {
        interval_ms = interval;
        anim_atlas = atlas;
    }
    ~Animation() = default;
    void Play(int x, int y, int delta)
    {
        timer += delta;
        if (timer >= interval_ms)
        {
            idx_frame = (idx_frame + 1) % anim_atlas->frame_list.size();
            timer = 0;
        }
        putimage_alpha(x, y, anim_atlas->frame_list[idx_frame]);
    }
};

class Player {
private:
    const int PLAYER_SPEED = 3;
    static const int PLAYER_WIDTH = 160;
    static const int PLAYER_HEIGHT = 160;
    const int SHADOW_WIDTH = 64;

    IMAGE img_shadow;
    Animation* anim_left;
    Animation* anim_right;
    POINT position = { 500,500 };
    bool is_move_up = false;
    bool is_move_down = false;
    bool is_move_left = false;
    bool is_move_right = false;
public:
    Player()
    {
        loadimage(&img_shadow, _T("img/shadow_player.png"));
        anim_left = new Animation(atlas_player_left, 100);
        anim_right = new Animation(atlas_player_right, 100);
    }
    ~Player()
    {
        delete anim_left;
        delete anim_right;
    }

    void ProcessEvent(const ExMessage& msg)
    {
        switch (msg.message)
        {
        case WM_KEYDOWN:
            switch (msg.vkcode)
            {
            case VK_UP:
                is_move_up = true;
                break;
            case VK_DOWN:
                is_move_down = true;
                break;
            case VK_LEFT:
                is_move_left = true;
                break;
            case VK_RIGHT:
                is_move_right = true;
                break;
            }
            break;
        case WM_KEYUP:
            switch (msg.vkcode)
            {
            case VK_UP:
                is_move_up = false;
                break;
            case VK_DOWN:
                is_move_down = false;
                break;
            case VK_LEFT:
                is_move_left = false;
                break;
            case VK_RIGHT:
                is_move_right = false;
                break;
            }
            break;
        }
    }

    void Move()
    {
        int dir_x = is_move_right - is_move_left;
        int dir_y = is_move_down - is_move_up;
        double len_dir = sqrt(dir_x * dir_x + dir_y * dir_y);
        if (len_dir != 0)
        {
            double normalized_x = dir_x / len_dir;
            double normalized_y = dir_y / len_dir;
            position.x += (int)(PLAYER_SPEED * normalized_x);
            position.y += (int)(PLAYER_SPEED * normalized_y);
        }
        if (position.x < 0) position.x = 0;
        if (position.y < 0) position.y = 0;
        if (position.x + PLAYER_WIDTH > WINDOW_WIDTH) position.x = WINDOW_WIDTH - PLAYER_WIDTH;
        if (position.y + PLAYER_HEIGHT > WINDOW_HEIGHT) position.y = WINDOW_HEIGHT - PLAYER_HEIGHT;
    }

    void Draw(int delta)
    {
        int pos_shadow_x = position.x + (PLAYER_WIDTH / 2 - SHADOW_WIDTH / 2) + 18;
        int pos_shadow_y = position.y + PLAYER_HEIGHT - 20;
        putimage_alpha(pos_shadow_x, pos_shadow_y, &img_shadow);

        if (is_move_left)
            anim_left->Play(position.x, position.y, delta);
        else if (is_move_right)
            anim_right->Play(position.x, position.y, delta);
        else
            anim_right->Play(position.x, position.y, delta);
    }

    const POINT& GetPosition()const
    {
        return position;
    }
    int GetWidth() const { return PLAYER_WIDTH; }
    int GetHeight() const { return PLAYER_HEIGHT; }
};

class Bullet
{
private:
    const int RADIUS = 10;
public:
    POINT position{ 0,0 };
    Bullet() = default;
    ~Bullet() = default;
    void Draw() const
    {
        setlinecolor(RGB(255, 155, 50));
        setfillcolor(RGB(200, 75, 10));
        fillcircle(position.x, position.y, RADIUS);
    }
};

class Enemy
{
private:
    const int SPEED = 2;
    const int FRAME_WIDTH = 160;
    const int FRAME_HEIGHT = 160;
    const int SHADOW_WIDTH = 96;
    IMAGE img_shadow;
    Animation* anim_left;
    Animation* anim_right;
    POINT position = { 0,0 };
    bool facing_left = false;
    bool alive = true;

    enum class SpawnEdge
    {
        Up = 0,
        Down,
        Left,
        Right
    };
public:
    Enemy()
    {
        loadimage(&img_shadow, _T("img/shadow_enemy.png"));
        anim_left = new Animation(atlas_enemy_left, 45);
        anim_right = new Animation(atlas_enemy_right, 45);

        SpawnEdge edge = (SpawnEdge)(rand() % 4);
        switch (edge)
        {
        case SpawnEdge::Up:
            position.x = rand() % WINDOW_WIDTH;
            position.y = -FRAME_HEIGHT;
            break;
        case SpawnEdge::Down:
            position.x = rand() % WINDOW_WIDTH;
            position.y = WINDOW_HEIGHT;
            break;
        case SpawnEdge::Left:
            position.x = -FRAME_WIDTH;
            position.y = rand() % WINDOW_HEIGHT;
            break;
        case SpawnEdge::Right:
            position.x = WINDOW_WIDTH;
            position.y = rand() % WINDOW_HEIGHT;
            break;
        default:
            break;
        }
    }

    bool CheckBulletCollision(const Bullet& bullet)
    {
        bool is_overlap_x = bullet.position.x >= position.x && bullet.position.x <= position.x + FRAME_WIDTH;
        bool is_overlap_y = bullet.position.y >= position.y && bullet.position.y <= position.y + FRAME_HEIGHT;
        return is_overlap_x && is_overlap_y;
    }

    bool CheckPlayerCollision(const Player& player)
    {
        POINT check_position = { position.x + FRAME_WIDTH / 2, position.y + FRAME_HEIGHT / 2 };
        int collision_width = player.GetWidth() * 0.7;
        int collision_height = player.GetHeight() * 0.7;
        int offset_x = (player.GetWidth() - collision_width) / 2;
        int offset_y = (player.GetHeight() - collision_height) / 2;

        bool is_overlap_x = check_position.x >= player.GetPosition().x + offset_x &&
            check_position.x <= player.GetPosition().x + offset_x + collision_width;
        bool is_overlap_y = check_position.y >= player.GetPosition().y + offset_y &&
            check_position.y <= player.GetPosition().y + offset_y + collision_height;
        return is_overlap_x && is_overlap_y;
    }

    void Move(const Player& player)
    {
        const POINT& player_position = player.GetPosition();
        int dir_x = player_position.x - position.x;
        int dir_y = player_position.y - position.y;
        double len_dir = sqrt(dir_x * dir_x + dir_y * dir_y);
        if (len_dir != 0)
        {
            double nx = dir_x / len_dir;
            double ny = dir_y / len_dir;
            position.x += (int)(SPEED * nx);
            position.y += (int)(SPEED * ny);
        }
        if (dir_x < 0)
            facing_left = true;
        else if (dir_x > 0)
            facing_left = false;
    }

    void Draw(int delta)
    {
        int pos_shadow_x = position.x + (FRAME_WIDTH / 2 - SHADOW_WIDTH / 2) + 25;
        int pos_shadow_y = position.y + FRAME_HEIGHT - 40;
        putimage_alpha(pos_shadow_x, pos_shadow_y, &img_shadow);
        if (facing_left)
            anim_left->Play(position.x, position.y, delta);
        else
            anim_right->Play(position.x, position.y, delta);
    }

    ~Enemy()
    {
        delete anim_left;
        delete anim_right;
    }
    void Hurt()
    {
        alive = false;
    }
    bool CheckAlive()
    {
        return alive;
    }
};

// Atlas class handles loading animation frames; no separate LoadAnimation required.

void TryGenerateEnemy(std::vector<Enemy*>& enemy_list)
{
    const int INTERVAL = 100;
    static int counter = 0;
    if ((++counter) % INTERVAL == 0)
    {
        enemy_list.push_back(new Enemy());
    }
}

void UpdateBullets(std::vector<Bullet>& bullet_list, const Player& player)
{
    const double RADIAL_SPEED = 0.0025;
    const double TANGENT_SPEED = 0.0035;
    double radian_interval = 2 * 3.14159 / bullet_list.size();
    POINT player_position = player.GetPosition();
    double radius = 80 + 50 * sin(GetTickCount() * RADIAL_SPEED);
    for (size_t i = 0; i < bullet_list.size(); i++)
    {
        double radian = GetTickCount() * TANGENT_SPEED + radian_interval * i;
        bullet_list[i].position.x = player_position.x + player.GetWidth() / 2 + (int)(radius * sin(radian));
        bullet_list[i].position.y = player_position.y + player.GetWidth() / 2 + (int)(radius * cos(radian));
    }
}

void DrawPlayerScore(int score)
{
    static TCHAR text[64];
    _stprintf_s(text, _T("SCORE:%d"), score);
    setbkmode(TRANSPARENT);
    settextcolor(RGB(255, 85, 185));
    outtextxy(10, 10, text);
}

int main()
{
    initgraph(WINDOW_WIDTH, WINDOW_HEIGHT);
    srand((unsigned int)time(NULL));
    atlas_player_left = new Atlas(_T("img/player_left_%d.png"), 40);
    atlas_player_right = new Atlas(_T("img/player_right_%d.png"), 40);
    atlas_enemy_left = new Atlas(_T("img/enemy_left_%d.png"), 32);
    atlas_enemy_right = new Atlas(_T("img/enemy_right_%d.png"), 32);

    mciSendString(_T("open mus/bgm.mp3 alias bgm"), NULL, 0, NULL);
    mciSendString(_T("open mus/hit.wav alias hit"), NULL, 0, NULL);

    int score = 0;
    Player player;
    ExMessage msg;
    IMAGE img_menu;
    IMAGE img_background;
    std::vector<Enemy*> enemy_list;
    std::vector<Bullet> bullet_list(3);

    RECT region_btn_start_game, region_btn_quit_game;
    region_btn_start_game.left = (WINDOW_WIDTH - BUTTON_WIDTH) / 2;
    region_btn_start_game.right = region_btn_start_game.left + BUTTON_WIDTH;
    region_btn_start_game.top = 430;
    region_btn_start_game.bottom = region_btn_start_game.top + BUTTON_HEIGHT;

    region_btn_quit_game.left = (WINDOW_WIDTH - BUTTON_WIDTH) / 2;
    region_btn_quit_game.right = region_btn_quit_game.left + BUTTON_WIDTH;
    region_btn_quit_game.top = 550;
    region_btn_quit_game.bottom = region_btn_quit_game.top + BUTTON_HEIGHT;

    StartGameButton btn_start_game(region_btn_start_game, _T("img/ui_start_idle.png"), _T("img/ui_start_hovered.png"), _T("img/ui_start_pushed.png"));
    QuitGameButton btn_quit_game(region_btn_quit_game, _T("img/ui_quit_idle.png"), _T("img/ui_quit_hovered.png"), _T("img/ui_quit_pushed.png"));

    loadimage(&img_menu, _T("img/menu.png"));
    loadimage(&img_background, _T("img/background.png"));

    BeginBatchDraw();
    while (running)
    {
        DWORD frame_start = GetTickCount();
        while (peekmessage(&msg))
        {
            if (!is_game_started)
            {
                btn_start_game.ProcessEvent(msg);
                btn_quit_game.ProcessEvent(msg);
            }
            else
            {
                player.ProcessEvent(msg);
            }
        }
        cleardevice();
        if (!is_game_started)
        {
            putimage(0, 0, &img_menu);
            btn_start_game.Draw();
            btn_quit_game.Draw();
        }
        else
        {
            putimage(0, 0, &img_background);
            TryGenerateEnemy(enemy_list);
            UpdateBullets(bullet_list, player);
            player.Move();

            //敌人移动&碰撞检测
            for (Enemy* enemy : enemy_list)
            {
                enemy->Move(player);
            }
            for (Enemy* enemy : enemy_list)
            {
                if (enemy->CheckPlayerCollision(player))
                {
                    static TCHAR text[128];
                    _stprintf_s(text, _T("SCORE:%d"), score);
                    MessageBox(GetHWnd(), _T("MISSION FAILED"), _T("GAME OVER"), MB_OK);
                    running = false;
                    break;
                }
            }
            //子弹命中敌人
            for (Enemy* enemy : enemy_list)
            {
                for (const Bullet& bullet : bullet_list)
                {
                    if (enemy->CheckBulletCollision(bullet))
                    {
                        mciSendString(_T("play hit from 0"), NULL, 0, NULL);
                        enemy->Hurt();
                        score++;
                    }
                }
            }
            //清除死亡敌人
            for (size_t i = 0; i < enemy_list.size(); i++)
            {
                Enemy* enemy = enemy_list[i];
                if (!enemy->CheckAlive())
                {
                    std::swap(enemy_list[i], enemy_list.back());
                    enemy_list.pop_back();
                    delete enemy;
                    i--;
                }
            }

            //绘制元素
            for (auto& bullet : bullet_list)
                bullet.Draw();
            int delta = GetTickCount() - frame_start;
            player.Draw(delta);
            for (Enemy* enemy : enemy_list)
                enemy->Draw(delta);
            DrawPlayerScore(score);
        }
        FlushBatchDraw();
        DWORD frame_end = GetTickCount();
        int frame_delay = 1000 / 60 - (frame_end - frame_start);
        if (frame_delay > 0)
            Sleep(frame_delay);
    }
    EndBatchDraw();

    //资源释放
    for (Enemy* enemy : enemy_list)
        delete enemy;
    delete atlas_player_left;
    delete atlas_player_right;
    delete atlas_enemy_left;
    delete atlas_enemy_right;
    closegraph();
    return 0;
}