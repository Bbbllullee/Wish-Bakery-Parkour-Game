#include <graphics.h>  //引入EasyX图形库,提供所有绘图和鼠标键盘输入函数
#include <string.h>
#include <conio.h>
#include <stdio.h>
#include <windows.h>   //Windows API,GetAsyncKeyState
#include <time.h>      //时间函数,srand((unsigned)time(NULL))初始化随机种子

//游戏常量
#define WIN_W     900         // 窗口宽
#define WIN_H     540         // 窗口高
#define GROUND_Y  480         // 地面y坐标固定在480,角色脚站在此线
#define ROLE_X    100         // 角色x坐标固定100,横版游戏中角色只上下跳不横移
#define GRAVITY   8           // 重力(像素/帧)控制下落速度
#define SPEED     16          // 障碍物/食物速度
#define TARGET    15          // 目标分数
#define _WIN32_WINNT 0x0600
#define FLY_DURATION   5000   //飞行持续时间
#define FLY_GEN_INTERVAL 120  // 飞行器生成间隔（帧数）

//素材图片
IMAGE img_jump;
IMAGE img_move;
IMAGE img_bricks;
IMAGE img_food[5];
IMAGE img_menuBg;    // 菜单背景
IMAGE img_gameBg;    // 游戏背景
IMAGE img_ground;    // 游戏地面
IMAGE img_flyM;      //飞行器

//各元素对应的掩码图
IMAGE img_jump_mask;
IMAGE img_move_mask;
IMAGE img_bricks_mask;
IMAGE img_food_mask[5];
IMAGE img_flyM_mask;

int roleW = 90;      // 角色宽
int roleH = 100;     // 角色高
int bricksW = 40;    // 障碍物宽
int bricksH = 55;    // 障碍物高
int foodW = 50;      // 食物宽
int foodH = 40;      // 食物高
int flymW = 70;      //飞行器宽
int flymH = 60;      //飞行器高
int flymAlive = 0;   //飞行是否存活
int roleFly = 0;     //角色是否飞行
ULONGLONG flyStartTime = 0;  //飞行开始时间

int score = 0;       //全局变量吃食物得分
int level = 1;       //当前关卡数, 每升一关速度加快
int l = 1;
int gameOver = 0;    //游戏结束标志, 1 = 触发了GameOver
int unlockedLevel = 1;  //已解锁的最高关卡

int roleY = GROUND_Y - 100;   //角色顶部y坐标初始值
int roleVy = 0;              //角色垂直速度,负值向上/正值向下
int isJumping = 0;           // 跳跃标志,1= 正在空中,防止二段跳

int bricksX = WIN_W;         //障碍物初始(窗口右边界外)
int bricksY = GROUND_Y - 55; //障碍物底部贴地

int flymX = WIN_W;
int flymY = 300;


// 用结构体声明5个食物的数组（金字塔形)
struct Food {
	IMAGE* img;   //指向食物图片的指针
	int x, y;     //食物左上角在屏幕上的坐标
	int alive;    //存活标志,1=存在可吃,0=已吃消失
} f[5];

//界面左上角设计
void show()
{
	TCHAR str[64];
	settextstyle(20, 0, _T("宋体"));      //设置文字字体
	settextcolor(BLACK);                  //设置文字颜色
	setbkmode(TRANSPARENT);               //设置文字背景透明, 不会被方框盖住

	_stprintf_s(str, _T("Score：%d"), score);//拼接文字和变量
	outtextxy(50, 20, str);               //在指定位置放上score

	_stprintf_s(str, _T("Target：%d"), TARGET);
	outtextxy(50, 45, str);               //在指定位置放上目标分数

	_stprintf_s(str, _T("Level：%d"), l);
	outtextxy(50, 65, str);               //在指定位置放上关卡数

	outtextxy(50, 85, _T("按space跳跃"));    //在指定位置放上“按空格跳跃”	

	// 显示飞行状态（第4关且飞行中）
	if (l == 4 && roleFly) {
		ULONGLONG remain = (FLY_DURATION - (GetTickCount() - flyStartTime)) / 1000;
		_stprintf_s(str, _T("飞行状态: %d秒"), remain + 1);
		settextcolor(RGB(255, 100, 0));
		outtextxy(50, 110, str);
	}
}

//加载资源
void loadResource()
{
	loadimage(&img_menuBg, _T("menuBg.jpg"));
	loadimage(&img_jump, _T("jump.png"), 90, 100);  //用来缩放图片  跳跃图片
	loadimage(&img_move, _T("move.png"), 90, 100);  //移动图片
	loadimage(&img_bricks, _T("bricks.png"), 40, 55);  //障碍物图片
	loadimage(&img_gameBg, _T("gameBg.jpg"));
	loadimage(&img_ground, _T("ground.jpg"));
	loadimage(&img_flyM, _T("flym.png"), 50, 40);

	loadimage(&img_jump_mask, _T("jump_mask.png"), 90, 100);
	loadimage(&img_move_mask, _T("move_mask.png"), 90, 100);
	loadimage(&img_bricks_mask, _T("bricks_mask.png"), 40, 55);
	loadimage(&img_flyM_mask, _T("flym_mask.png"), 50, 40);

	TCHAR foodnum[32], foodmask[64];
	for (int i = 0; i < 5; i++) {
		_stprintf_s(foodnum, _T("food%d.png"), i + 1);
		loadimage(&img_food[i], foodnum, 50, 40);
		_stprintf_s(foodmask, _T("food%d_mask.png"), i + 1);
		loadimage(&img_food_mask[i], foodmask, 50, 40);
	}

	// 调试输出:在控制台打印尺寸
	printf("[loadResource] role=%d x %d, bricks=%d x %d, food=%d x %d\n",
		roleW, roleH, bricksW, bricksH, foodW, foodH);
}

//开始界面 返回 1 = 开始游戏, 0 = 退出
int showStart()
{
	//先画开始界面背景
	putimage(0, 0, &img_menuBg);

	//标题居中
	settextstyle(56, 0, _T("微软雅黑"));
	settextcolor(RGB(255, 230, 240));  //淡粉色
	setbkmode(TRANSPARENT);
	LPCTSTR title = _T("Wish Bakery Parkour Game");
	int tw = textwidth(title);
	outtextxy((WIN_W - tw) / 2, 60, title);

	// 两个按钮
	int bw = 180, bh = 70;
	int bx1 = 180, bx2 = 540, by = 280;

	settextcolor(RGB(255, 120, 170));            //粉色文字
	settextstyle(36, 0, _T("微软雅黑"));
	LPCTSTR startText = _T("MENU");
	LPCTSTR exitText = _T("EXIT");
	tw = textwidth(startText);
	outtextxy(bx1 + (bw - tw) / 2, by + (bh - 36) / 2, startText);
	tw = textwidth(exitText);
	outtextxy(bx2 + (bw - tw) / 2, by + (bh - 36) / 2, exitText);

	//游戏说明放到最下面,居中
	settextstyle(22, 0, _T("微软雅黑"));
	settextcolor(RGB(120, 80, 100));
	LPCTSTR tip = _T("* 按空格跳跃,躲避障碍物,吃食物得分 *");
	tw = textwidth(tip);
	outtextxy((WIN_W - tw) / 2, 470, tip);    //最下面

	//检测鼠标点击位置
	while (1) {
		if (MouseHit()) {
			MOUSEMSG m = GetMouseMsg();
			if (m.uMsg == WM_LBUTTONDOWN) {
				if (m.x >= bx1 && m.x <= bx1 + bw &&
					m.y >= by && m.y <= by + bh) {
					return 1;
				}
				if (m.x >= bx2 && m.x <= bx2 + bw &&
					m.y >= by && m.y <= by + bh)
					return 0;
			}
		}
		if (GetAsyncKeyState(VK_ESCAPE) & 0x8000)
			return 0;
		Sleep(30);
	}
}

//碰撞检测：经典 AABB(Axis-Aligned Bounding Box,轴对齐包围盒) 算法
int hit(int roleX, int roleY, int roleW, int roleH,
	int bricksX, int bricksY, int bricksW, int bricksH)
{
	if (roleX < bricksX + bricksW && roleX + roleW > bricksX &&
		roleY < bricksY + bricksH && roleY + roleH > bricksY)
		return 1;  // 发生碰撞
	return 0;      // 未碰撞
}

//游戏菜单界面
int showMenu()
{
	putimage(0, 0, &img_gameBg);  //菜单背景

	setbkcolor(TRANSPARENT);  //设置文字背景色为透明

	//显示GAME MENU
	settextstyle(50, 0, _T("Harlow Solid Italic"));
	settextcolor(RGB(255, 80, 100));
	LPCTSTR t = _T("GAME MENU");
	int tw = textwidth(t);
	outtextxy((WIN_W - tw) / 2, 100, t);

	//显示游戏关卡：只显示已解锁的关卡
	int btnWidth = 120, btnHeight = 60;
	int startX = 150, y = 280;
	settextcolor(RGB(0, 35, 102));
	settextstyle(24, 0, _T("Kristen ITC"));

	LPCTSTR labels[4] = { _T("level1"), _T("level2"), _T("level3"), _T("level4") };
	int xs[4] = { startX, startX + 170, startX + 340, startX + 510 };

	// 始终显示4个关卡文字，但未解锁的用灰色显示
	for (int i = 0; i < 4; i++) {
		// 未解锁的关卡用灰色显示
		if (i >= unlockedLevel) {
			settextcolor(RGB(150, 150, 150));  // 灰色
		}
		else {
			settextcolor(RGB(0, 35, 102));     // 正常颜色
		}
		tw = textwidth(labels[i]);
		outtextxy(xs[i] + (btnWidth - tw) / 2, y + (btnHeight - 24) / 2, labels[i]);
	}

	// 等待鼠标点击按钮
	while (1) {
		if (MouseHit()) {
			MOUSEMSG m = GetMouseMsg();
			if (m.uMsg == WM_LBUTTONDOWN) {
				for (int i = 0; i < unlockedLevel; i++) {
					if (m.x >= xs[i] && m.x <= xs[i] + btnWidth &&
						m.y >= y && m.y <= y + btnHeight)
						return i + 1;
				}
			}
		}
		Sleep(50);// 防止 CPU 100%
	}

}

//游戏结束界面
int showGameOverScreen()
{
	putimage(0, 0, &img_gameBg);
	//设置文字背景色为透明
	setbkcolor(TRANSPARENT);

	//显示 GAME OVER 标题
	settextstyle(50, 0, _T("微软雅黑"));
	settextcolor(RGB(255, 80, 100));
	LPCTSTR t = _T("GAME OVER");
	int tw = textwidth(t);
	outtextxy((WIN_W - tw) / 2, 100, t);

	//根据是否达到目标分数显示不同文字
	settextstyle(25, 0, _T("微软雅黑"));
	if (score >= TARGET) {
		settextcolor(RGB(255, 150, 50));
		LPCTSTR n = _T("完成目标分数!解锁下一关");
		tw = textwidth(n);
		outtextxy((WIN_W - tw) / 2, 180, n);
	}
	else {
		settextcolor(RGB(255, 80, 100));
		TCHAR failMsg[64];
		_stprintf_s(failMsg, _T("目标分数未达成! 还需 %d 分"), TARGET - score);
		tw = textwidth(failMsg);
		outtextxy((WIN_W - tw) / 2, 180, failMsg);
	}

	//设置按钮:Restart,Home,Menu;并让按钮靠中间对齐
	int btnWidth = 120, btnHeight = 60;
	int startX = 180, y = 300;
	settextcolor(RGB(0, 35, 102));
	settextstyle(24, 0, _T("微软雅黑"));

	//根据是否达到目标分数决定按钮数量
	int btnCount;
	int xs[4];

	if (score >= TARGET) {
		//达到目标：显示4个按钮
		btnCount = 4;
		LPCTSTR labels[4] = { _T("Restart"), _T("Home"), _T("Menu"), _T("Next") };
		xs[0] = startX;
		xs[1] = startX + 170;
		xs[2] = startX + 340;
		xs[3] = startX + 510;

		for (int i = 0; i < btnCount; i++) {
			tw = textwidth(labels[i]);
			outtextxy(xs[i] + (btnWidth - tw) / 2, y + (btnHeight - 24) / 2, labels[i]);
		}
	}
	else {
		//未达到目标：只显示3个按钮
		btnCount = 3;
		LPCTSTR labels[3] = { _T("Restart"), _T("Home"), _T("Menu") };
		xs[0] = startX + 60;
		xs[1] = startX + 230;
		xs[2] = startX + 400;

		for (int i = 0; i < btnCount; i++) {
			tw = textwidth(labels[i]);
			outtextxy(xs[i] + (btnWidth - tw) / 2, y + (btnHeight - 24) / 2, labels[i]);
		}
	}

	// 等待鼠标点击按钮
	while (1) {
		if (MouseHit()) {
			MOUSEMSG m = GetMouseMsg();
			if (m.uMsg == WM_LBUTTONDOWN) {
				for (int i = 0; i < btnCount; i++) {
					if (m.x >= xs[i] && m.x <= xs[i] + btnWidth &&
						m.y >= y && m.y <= y + btnHeight)
						return i + 1;
				}
			}
		}
		Sleep(50);
	}
	closegraph();
}

//重置关卡 
void resetLevel() {
	score = 0;
	roleY = GROUND_Y - roleH;
	roleVy = 0;
	isJumping = 0;
	bricksX = WIN_W;
	roleFly = 0;
	flymAlive = 0;
	flymX = WIN_W;
	flyStartTime = 0;

	//金字塔形的一组食物
	int baseX = WIN_W + 200;//最后一个食物的初始x位置
	int ys[5] = { 260,220, 180,220,260 };  //食物高度（呈金字塔形）
	for (int i = 0; i < 5; i++) {
		f[i].img = &img_food[i];
		f[i].x = baseX + i * (foodW + 10);
		f[i].y = ys[i];
		f[i].alive = 1;
	}
	gameOver = 0;
}//主游戏循环
int gameLoop()       //0 返回开始界面    1 返回关卡选择
{
	resetLevel();                              //初始化关卡
	int speed;
	// 用于飞行器生成的帧计数器
	static int flyGenCounter = 0;

	while (1) {                                //外层大循环,保证游戏运行期间不退出
		ULONGLONG t = GetTickCount();              //记录本帧开始时刻(毫秒),用于帧率控制

		BeginBatchDraw();

		putimage(0, 0, &img_gameBg);           //画游戏背景
		putimage(0, GROUND_Y, &img_ground);    //画游戏地面
		show();                                //在界面左上角显示分数/关卡/操作提示

		//  飞行状态处理
		if (roleFly) {
			// 检查飞行时间是否结束
			if (GetTickCount() - flyStartTime >= FLY_DURATION) {
				roleFly = 0;  // 飞行结束
				// 如果飞行结束时在空中，设置为跳跃状态让其下落
				if (roleY < GROUND_Y - roleH) {
					isJumping = 1;
					roleVy = 0;
				}
			}
		}

		// 角色移动逻辑（区分飞行和普通状态）
		if (roleFly) {
			// 飞行状态：按空格上升，不按则缓慢下降
			if (GetAsyncKeyState(' ') & 0x8000) {
				roleY -= 6;  // 上升速度
				if (roleY < 50) roleY = 50;  // 限制最高高度
			}
			else {
				roleY += 3;  // 缓慢下降
			}
			// 限制最低不能穿过地面
			if (roleY > GROUND_Y - roleH) {
				roleY = GROUND_Y - roleH;
				roleFly = 0;  // 落地后飞行状态结束
				isJumping = 0;
			}
			// 飞行时使用跳跃图片
			putimage(ROLE_X, roleY, &img_jump_mask, SRCAND);
			putimage(ROLE_X, roleY, &img_jump, SRCPAINT);
		}
		else {
			// 普通状态：跳跃和重力逻辑
			if ((GetAsyncKeyState(' ') & 0x8000) && !isJumping) {
				isJumping = 1;
				roleVy = -70;
			}
			if (isJumping || roleY < GROUND_Y - roleH) {
				roleVy += GRAVITY;
				roleY += roleVy;
				if (roleY >= GROUND_Y - roleH) {
					roleY = GROUND_Y - roleH;
					roleVy = 0;
					isJumping = 0;
				}
			}
			// 普通状态使用移动图片
			putimage(ROLE_X, roleY, &img_move_mask, SRCAND);
			putimage(ROLE_X, roleY, &img_move, SRCPAINT);
		}

		// 第4关：飞行器生成和移动
		if (l == 4) {
			// 生成飞行器（不在飞行状态时才能生成，避免无限生成）
			if (!roleFly) {
				flyGenCounter++;
				if (flyGenCounter >= FLY_GEN_INTERVAL && flymAlive == 0) {
					flymX = WIN_W;
					flymY = rand() % 201 + 100;  // 随机Y坐标 100~300
					flymAlive = 1;
					flyGenCounter = 0;
				}
			}

			// 移动飞行器
			if (flymAlive) {
				flymX -= SPEED;  // 飞行器移动速度
				putimage(flymX, flymY, &img_flyM_mask, SRCAND);
				putimage(flymX, flymY, &img_flyM, SRCPAINT);

				// 碰撞检测：吃到飞行器
				if (hit(ROLE_X, roleY, roleW, roleH, flymX, flymY, flymW, flymH)) {
					flymAlive = 0;
					roleFly = 1;
					flyStartTime = GetTickCount();
					// 吃到飞行器后重置跳跃状态
					isJumping = 0;
					roleVy = 0;
				}

				// 飞行器移出屏幕
				if (flymX + flymW < 0) {
					flymAlive = 0;
				}
			}
		}
		else {
			// 非第4关，重置飞行器相关变量
			flymAlive = 0;
			flyGenCounter = 0;
		}

		//障碍物的移动
		if (l == 3 || l == 4) {
			srand(time(NULL));
			bricksX -= rand() % 21 + 10;
		}
		else {
			bricksX -= SPEED;
		}
		if (bricksX + bricksW < 0)                     //障碍物完全移出屏幕左侧
			bricksX = WIN_W + rand() % 200;            //重置到屏幕右侧外，加上0~199的随机偏移
		putimage(bricksX, bricksY, &img_bricks_mask, SRCAND);
		putimage(bricksX, bricksY, &img_bricks, SRCPAINT);

		//食物组的移动
		int groupOut = 1;                              //假设整组都出界
		for (int i = 0; i < 5; i++) {
			if (f[i].alive) groupOut = 0;
			f[i].x -= SPEED;
		}
		int rightMost = -999;
		for (int i = 0; i < 5; i++)
			if (f[i].x + foodW > rightMost) rightMost = f[i].x + foodW;

		if (l == 1) {
			if (rightMost < 0 || groupOut) {
				int baseX = WIN_W + 200 + rand() % 100;
				int ys[5] = { 260,220, 180,220,260 };
				for (int i = 0; i < 5; i++) {
					f[i].x = baseX + i * (foodW + 10);
					f[i].y = ys[i];
					f[i].alive = 1;
				}
			}
		}
		else {
			//随机生成食物（后三关设计）
			int limity1 = rand() % (300 - 180 + 1) + 180;  //生成y范围（180<=limity<=300）
			int limity2 = rand() % (300 - 180 + 1) + 180;
			int limity3 = rand() % (300 - 180 + 1) + 180;
			int limity4 = rand() % (300 - 180 + 1) + 180;
			int limity5 = rand() % (300 - 180 + 1) + 180;
			if (rightMost < 0 || groupOut) {
				int baseX = WIN_W + 200 + rand() % 100;
				int ys[5] = { limity1,limity2,limity3,limity4,limity5 };
				for (int i = 0; i < 5; i++) {
					f[i].x = baseX + i * (foodW + 10);
					f[i].y = ys[i];
					f[i].alive = 1;
				}
			}
		}

		//画食物并进行碰撞检测，计算得分
		for (int i = 0; i < 5; i++) {
			if (!f[i].alive) continue;
			putimage(f[i].x, f[i].y, &img_food_mask[i], SRCAND);
			putimage(f[i].x, f[i].y, f[i].img, SRCPAINT);
			if (hit(ROLE_X, roleY, roleW, roleH, f[i].x, f[i].y, foodW, foodH)) {
				f[i].alive = 0;
				score++;
			}
		}
		//碰到障碍物，游戏结束
		if (hit(ROLE_X, roleY, roleW, roleH, bricksX, bricksY, bricksW, bricksH))
			gameOver = 1;

		//达到目标分数，下一关
		if (score >= TARGET) {
			EndBatchDraw();
			cleardevice();
			gameOver = 1;

		}

		EndBatchDraw();

		//若游戏结束，弹出选择弹窗
		if (gameOver) {
			// 如果达到目标分数，先更新解锁等级（无论点击什么按钮）
			if (score >= TARGET) {
				if (l + 1 > unlockedLevel && l < 4) {
					unlockedLevel = l + 1;
				}
			}

			cleardevice();
			int choice = showGameOverScreen();

			if (choice == 1) {
				// Restart: 重新开始当前关卡
				resetLevel();
				gameOver = 0;
				flyGenCounter = 0;
				continue;
			}
			else if (choice == 2) {
				// Home: 返回开始界面
				return 0;
			}
			else if (choice == 3) {
				// Menu: 返回关卡选择
				int s = showMenu();
				if (s >= 1 && s <= unlockedLevel) {
					l = s;
					resetLevel();
					gameOver = 0;
					flyGenCounter = 0;
					continue;
				}
				else {
					return 1;
				}
			}
			else if (score >= TARGET && choice == 4) {
				// Next: 下一关
				if (l < 4) {
					l++;
					resetLevel();
					gameOver = 0;
					flyGenCounter = 0;
					continue;
				}
				else {
					// 已经完成第4关，显示通关恭喜界面
					cleardevice();
					putimage(0, 0, &img_gameBg);
					settextstyle(50, 0, _T("微软雅黑"));
					settextcolor(RGB(255, 200, 50));
					LPCTSTR winTitle = _T("恭喜通关！");
					int tw = textwidth(winTitle);
					outtextxy((WIN_W - tw) / 2, 150, winTitle);

					settextstyle(25, 0, _T("微软雅黑"));
					settextcolor(RGB(100, 200, 100));
					LPCTSTR tip = _T("点击任意处返回主菜单");
					tw = textwidth(tip);
					outtextxy((WIN_W - tw) / 2, 350, tip);

					FlushBatchDraw();

					while (1) {
						if (MouseHit()) {
							MOUSEMSG m = GetMouseMsg();
							if (m.uMsg == WM_LBUTTONDOWN) {
								break;
							}
						}
						Sleep(50);
					}
					return 0;
				}
			}
		}
		if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) return 1;

		ULONGLONG dt = GetTickCount() - t;
		if (dt < 33) Sleep(33 - dt);
	}
}

int main() {
	srand((unsigned)time(NULL));
	initgraph(WIN_W, WIN_H);         //打开窗口
	loadResource();                  //加载资源

	unlockedLevel = 1;  // 初始只解锁第一关

	while (1) {
		int c = showStart();             //开始界面选择，1进入Menu界面开始游戏，0退出游戏
		if (c == 0) break;           //退出游戏

		while (1) {
			int s = showMenu();                 //显示关卡选择菜单
			if (s >= 1 && s <= unlockedLevel) {
				l = s;
				int result = gameLoop();
				if (result == 0) {
					break;
				}

			}
		}
	}
	closegraph();
	return 0;
}
