/*
Conway's Game of Life implementation with both classical
and modified rules. Modified rules keep large enough (500x500+) field always alive,
with new activity emerging here and there. Small field could die out completely but can't
stuck in a still state.

Modified rules (on top of classical ones):
1. If cell is alive for 100 cycles, it has to either die or reborn (age is reset to 1).
Chance of death is set to 1%.
2. If cell is dead but for 100 cycles had at least one alive neighbour, then if it has
exactly 2 neighbours, it has 1% chance of becoming alive. Otherwise, counter is reset to 0.

Designed by Dmitry Dziuba from Ultimate Robotics.
*/

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <sys/time.h>

SDL_Surface *scr;
SDL_Window *screen;
SDL_Renderer *renderer;
SDL_Texture *scrt;
TTF_Font *font = NULL; 

void prepare_sdl2(int w, int h)
{
	SDL_Init(SDL_INIT_VIDEO);
	screen = SDL_CreateWindow("Life",
                          SDL_WINDOWPOS_UNDEFINED,
                          SDL_WINDOWPOS_UNDEFINED,
                          w, h,
                          0);		
	renderer = SDL_CreateRenderer(screen, -1, 0);
	
	scrt = SDL_CreateTexture(renderer,
                               SDL_PIXELFORMAT_ARGB8888,
                               SDL_TEXTUREACCESS_STREAMING,
                               w, h);	
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

	SDL_RenderClear(renderer);
	SDL_RenderPresent(renderer);	
}


int FX, FY;
int *state1;
int *state2;
int *state;

int *state_sum;
int *state_tmp;

float *trace;

int rand_rules = 1;

int draw_trace = 1;

void field_init(int x, int y)
{
	FX = x;
	FY = y;
	state1 = new int[FX*FY];
	state2 = new int[FX*FY];

	state_sum = new int[FX*FY];
	state_tmp = new int[FX*FY];
	
	trace = new float[FX*FY];
	for(int xx = 0; xx < FX*FY; xx++)
	{
		state1[xx] = 0;
		trace[xx] = 0;
	}
	state = state1;
}

//setting initial state, can be anything
void field_rand()
{
	int N = 20;
	for(int x = FX/2-N; x < FX/2+N; x++)
		for(int y = FY/2-N; y < FY/2+N; y++)
			state[y*FX+x] = (rand()%3)==1;
}


//step calculation with somewhat optimized math, 
//suprizingly this gives sinificant speed increase (about 30%)
void field_step()
{
	int *state_n = state2; //state points to current state, state_n - to the next one
	if(state == state2) //state1 and state2 are used as current/next without moving content,
		state_n = state1; //instead pointer is switched

	int *st = state_tmp;
	for(int x = 0; x < FX*FY; x++)
		st[x] = state[x]>0; //for simplifying further calculations, all ages are mapped into 0 or 1
	
	for(int x = 1; x < FX-1; x++) //for all non-edge cells sum of 8 neighbours in explicit for speed increase
		for(int y = 1; y < FY-1; y++)
		{
			int idx = y*FX+x;
			state_sum[idx] = st[idx-1] + st[idx+1];
			state_sum[idx] += st[idx-FX-1] + st[idx-FX] + st[idx-FX+1];
			state_sum[idx] += st[idx+FX-1] + st[idx+FX] + st[idx+FX+1];
		}

	for(int x = 0; x < FX; x++) //for edge cells, use full +-1 cycle on X, and 0 and (max-1) on Y
		for(int y = 0; y < FY; y += FY-1)
		{
			int sum = 0;
			for(int ddx = -1; ddx <= 1; ddx++)
				for(int ddy = -1; ddy <= 1; ddy++)
				{
					int dx = ddx, dy = ddy;
					if(dx == 0 && dy == 0) continue; //skip itself
					if(x+dx < 0) dx = FX-1; //wrap edges: 0 is neighbour to field size-1
					if(y+dy < 0) dy = FY-1;
					if(x+dx >= FX) dx = 1 - FX;
					if(y+dy >= FY) dy = 1 - FY;

					int idx = (y+dy)*FX + x+dx;
					sum += st[idx];
				}
			state_sum[y*FX+x] = sum;
		}

	for(int x = 0; x < FX; x+=FX-1) //same cycle but for all Y, and 0, (max-1) for X
		for(int y = 0; y < FY; y++)
		{
			int sum = 0;
			for(int ddx = -1; ddx <= 1; ddx++)
				for(int ddy = -1; ddy <= 1; ddy++)
				{
					int dx = ddx, dy = ddy;
					if(dx == 0 && dy == 0) continue;
					if(x+dx < 0) dx = FX-1;
					if(y+dy < 0) dy = FY-1;
					if(x+dx >= FX) dx = 1 - FX;
					if(y+dy >= FY) dy = 1 - FY;

					int idx = (y+dy)*FX + x+dx;
					sum += st[idx];
				}
			state_sum[y*FX+x] = sum;
		}
		
		
	for(int x = 0; x < FX; x++)
		for(int y = 0; y < FY; y++)
		{
			int sum = 0;
			int idx = y*FX + x;
			sum = state_sum[idx];
			state_n[idx] = state[idx];
			//standard rules:
			if(sum < 2) state_n[idx] = 0;
			if(sum == 3) state_n[idx] = 1;
			if(sum > 3) state_n[idx] = 0;
			if(rand_rules) //modified rules:
			{
				if(state_n[idx]<=0 && state[idx]<=0) //if cell stays empty
				{
					state_n[idx] = state[idx]-1; //increase negative age
					if(sum == 0) state_n[idx] = 0; //if has no neighbours at all - set zero age
					if(state_n[idx] < -100) //otherwise, if it's too negative old:
					{
						if(sum == 2) //if it has exatcly 2 neighbours, life is born with 1% chance
						{
							if((rand()%100) == 99)
								state_n[idx] = 1;
							else
								state_n[idx] = 0; //or age is reset
						}
						else //otherwise negative age is simply reset
							state_n[idx] = 0;
					}
				}
				if(state_n[idx]>0 && state[idx]>0) //if cells keeps staying alive
				{
					state_n[idx] = state[idx]+1; //increase positive age
					if(state_n[idx] > 100) //if it's too old, there are 2 possibilities
					{
						if((rand()%100) == 99) //with 1% chance it dies
							state_n[idx] = 0;
						else
							state_n[idx] = 1; //otherwise, it's reborn, age is reset to 1
					}
				}
			}
			trace[idx] *= 0.99;
			trace[idx] += 0.01*(state_n[idx]>0);
		}

	state = state_n;
}

//convert activity map value into color: new cells are green, others - 
//lower value is blue, higher value is pink
int vv2col(float vv, int age)
{
	int r = 0, g = 0, b = 0;
	float t0 = 0.0005;
	float t1 = 0.008;
	float t2 = 0.2;
	if(vv < t0)// || age <= 0)
	{
		return 0;
	}
	if(age < 15 && age > 0)
	{
		if(vv < t1)
		{
			g = vv/t1*255;
		}
		else if(vv < t2)
			g = 255;
	}
	if(g == 0)
	{	
		if(vv < t2)
		{
			r = 0;
			g = 0;
			b = 255/t2*vv;
		}
		else
		{
			r = 255*vv*2;
			if(r > 255) r = 255;
			g = 0;//255*vv;
			b = 255 - (vv-t2)*10;
			if(b < 0) b = 0;
		}
		
	}
	return r | (g<<8) | (b<<16);
}

//convert age to color: new cells are green,
//medium age - blue, old - pink, ones close to death or rebirth - red
int age2col(float st)
{
	float T1 = 3, T2 = 15, T3 = 50, T4 = 100;
	int r = 0, g = 0, b = 0;
	if(st <= 0) return 0;
	if(st < T1) g = 255;
	else if(st < T2)
	{
		float vv = (st - T1) / (T2 - T1);
		g = 255 * (1.0 - vv);
		b = 255 * vv;
	}
	else if(st < T3)
		b = 255;
	else if(st < T4)
	{
		float vv = (st - T3) / (T4 - T3);
		b = 255 * (1.0 - vv);
		r = 255 * vv;
	}
	else 
		r = 255;
	return r | (g<<8) | (b<<16);
}

//draw into array draw_pix of size w x h, with zoom factor zm, and shits sx, sy
void draw_field(uint8_t *draw_pix, int w, int h, float zm, float sx, float sy)
{
	int DX = sx*zm, DY = sy*zm;
	int r = 0, g = 0, b = 0;
	
	int RX = FX*zm;
	int RY = FY*zm;
	if(RX > w) RX = w;
	if(RY > h) RY = h;
	
	float zm1 = 1.0 / zm;
	
	for(int x = 0; x < RX; x++)
		for(int y = 0; y < RY; y++)
		{
			float fx0 = (float)(x+DX)*zm1;
			float fy0 = (float)(y+DY)*zm1;
			int x0 = (x+DX)*zm1;
			int y0 = (y+DY)*zm1;
			if(x0 < 0 || x0 > FX) continue;
			if(y0 < 0 || y0 > FY) continue;
			int x1 = x0+1;
			int y1 = y0+1;
			if(x1 >= FX) x1 = 0;
			if(y1 >= FY) y1 = 0;
			
			float xv = fx0 - x0;
			float yv = fy0 - y0;
			
			float st0 = trace[y0*FX + x0];
			float stX = trace[y0*FX + x1];
			float stY = trace[y1*FX + x0];
			float stXY = trace[y1*FX + x1];
			
			
			float age0 = state[y0*FX + x0];
			float ageX = state[y0*FX + x1];
			float ageY = state[y1*FX + x0];
			float ageXY = state[y1*FX + x1];
			
			if(age0 < 0) age0 = 0;
			if(ageX < 0) ageX = 0;
			if(ageY < 0) ageY = 0;
			if(ageXY < 0) ageXY = 0;
			int max_age = age0;
			if(ageX > max_age) max_age = ageX;
			if(ageY > max_age) max_age = ageY;
			if(ageXY > max_age) max_age = ageXY;
			
			float stxx = st0*(1.0 - xv) + stX*xv;
			float styy = stY*(1.0 - xv) + stXY*xv;
			float st = stxx*(1.0-yv) + styy*yv;

			float agexx = age0*(1.0 - xv) + ageX*xv;
			float ageyy = ageY*(1.0 - xv) + ageXY*xv;
			int age = agexx*(1.0-yv) + ageyy*yv;
			if(max_age > 5 && age < 2) age = 0;

			int col = vv2col(st, age);

			if(!draw_trace)
			{
				int st = state[y0*FX + x0];
				col = age2col(st);
			}
			
			r = col&0xFF;
			g = (col>>8)&0xFF;
			b = (col>>16)&0xFF;

			int didx = y*w + x;
			didx *= 4;
			draw_pix[didx + 2] = r * (st > 0);
			draw_pix[didx + 1] = g * (st > 0);
			draw_pix[didx + 0] = b * (st > 0);
			draw_pix[didx + 3] = 0xFF;
			
		}
	
	return;	
}

int main(int argc, char **argv)
{
	SDL_Surface *msg = NULL;
	
	int w = 1200;
	int h = 700;
	
	field_init(w/2, h/2);
	field_rand();
	
	uint8_t *draw_pix = new uint8_t[w*h*4];
	prepare_sdl2(w, h);
	//text output
	SDL_Color textColor = { 255, 255, 255 }; 	
	TTF_Init();
	font = TTF_OpenFont( "../Ubuntu-R.ttf", 14 ); //filename, wanted font size
	if(font == NULL ) 
	{
		font = TTF_OpenFont( "Ubuntu-R.ttf", 14 ); //filename, wanted font size
		if(font == NULL ) 
			printf("can't load font!\n");
	}
	timeval curTime, prevTime, zeroTime;
	gettimeofday(&prevTime, NULL);
	gettimeofday(&zeroTime, NULL);

	int mbtn_l = 0, mbtn_r = 0, mbtn_m = 0;
	
	int need_send = 0, done = 0;
	int pause = 0;
	
	float zoom = 2;
	int tim_scale = 1;


	float kdx = 0; //dx, dy set via keyboard or mouse
	float kdy = 0;

	int kstate[1024];
	
	for(int x = 0; x < 1024; x++)
		kstate[x] = 0;
	
	int pre_mx = 0;
	int pre_my = 0;
	
	while( !done ) 
	{ 
//===========INPUT PROCESSING==================================
		int mouse_x = 0, mouse_y = 0;
		SDL_Event event;
		while( SDL_PollEvent( &event ) ) 
		{ 
			if( event.type == SDL_QUIT ) 
			{
				done = 1; 
				printf("SDL quit message\n");
			} 
			if( event.type == SDL_KEYDOWN ) 
			{
				if(event.key.keysym.sym == SDL_SCANCODE_ESCAPE) 
				{ 
					done = 1;
					printf("esc pressed\n");
				} 
				if(event.key.keysym.sym == SDL_SCANCODE_SPACE) 
				{ 
					pause = !pause;
				}
			}
		} 

//==============MOUSE==========================================
		uint8_t mstate;
		mstate = SDL_GetMouseState(&mouse_x, &mouse_y);

		if (mstate & SDL_BUTTON (1))
		{
			if(mouse_x > pre_mx)
				kdx++;
			if(mouse_x < pre_mx)
				kdx--;
			if(mouse_y > pre_my)
				kdy++;
			if(mouse_y < pre_my)
				kdy--;
		}

		pre_mx = mouse_x;
		pre_my = mouse_y;

		int lclk = 0;
		if (mstate & SDL_BUTTON (1))
		{
			if(mbtn_l == 0) lclk = 1;
			mbtn_l = 1;
		}
		else
		{
			mbtn_l = 0;
		}
		if (mstate & SDL_BUTTON (2)) mbtn_m = 1;
		if (mstate & SDL_BUTTON (3)) mbtn_r = 1;
		
		if(lclk)
		{
			;
		}
//==============MOUSE END======================================

//==============KEYBOARD==========================================
		int knum;
		uint8_t *keys = (uint8_t *)SDL_GetKeyboardState( &knum);
		if(knum > 1024) knum = 1024;
		for(int kk = 0; kk < knum; kk++)
		{
			if(keys[kk] != kstate[kk])
			{
				if(keys[SDL_SCANCODE_SPACE])
					pause = !pause;
				if(pause && keys[SDL_SCANCODE_S])
					field_step();
				if(keys[SDL_SCANCODE_D])
					draw_trace = !draw_trace;
			}
			kstate[kk] = keys[kk];
		}
		
		if(keys[SDL_SCANCODE_ESCAPE])
			done = 1;
		if(keys[SDL_SCANCODE_1]) 
			zoom = 1;
		if(keys[SDL_SCANCODE_2]) 
		{
			zoom = 2;
			kdx = 0;
			kdy = 0;
		}
		if(keys[SDL_SCANCODE_3]) 
		{
			//math is required only to keep center in the same place while zooming
			float kcx = (-kdx*(zoom) + FX/2*zoom);
			float kcy = (-kdy*(zoom) + FY/2*zoom);
			
			zoom *= 1.03;
			if(zoom > 16) zoom = 16;

			float ncx = (-kdx*(zoom) + FX/2*zoom);
			float ncy = (-kdy*(zoom) + FY/2*zoom);
			
			float ddx = ncx - kcx;
			float ddy = ncy - kcy;
			kdx += ddx/zoom;
			kdy += ddy/zoom;
		}
		if(keys[SDL_SCANCODE_4]) 
		{
			//same with zoom out
			float kcx = (-kdx*(zoom) + FX/2*zoom);
			float kcy = (-kdy*(zoom) + FY/2*zoom);

			zoom *= 0.97;
			if(zoom < 2) zoom = 2;

			float ncx = (-kdx*(zoom) + FX/2*zoom);
			float ncy = (-kdy*(zoom) + FY/2*zoom);
			
			float ddx = ncx - kcx;
			float ddy = ncy - kcy;
			kdx += ddx/(zoom*0.97);
			kdy += ddy/(zoom*0.97);
		}
		if(keys[SDL_SCANCODE_5]) 
			zoom = 4;

		if(keys[SDL_SCANCODE_Q]) 
			tim_scale = 11; //11 field steps per draw step
		if(keys[SDL_SCANCODE_A]) 
			tim_scale = 1; //1 field step per draw step
			
		if(keys[SDL_SCANCODE_R]) 
			rand_rules = 1; //modified rules
		if(keys[SDL_SCANCODE_N]) 
			rand_rules = 0; //classical rules

		if(keys[SDL_SCANCODE_UP]) 
			kdy -= 2;
		if(keys[SDL_SCANCODE_DOWN]) 
			kdy += 2;

		if(keys[SDL_SCANCODE_LEFT]) 
			kdx -= 2;
		if(keys[SDL_SCANCODE_RIGHT]) 
			kdx += 2;

//==============KEYBOARD END======================================
		
//===========INPUT PROCESSING END==============================

		gettimeofday(&curTime, NULL);
		int dT = (curTime.tv_sec - prevTime.tv_sec) * 1000000 + (curTime.tv_usec - prevTime.tv_usec);
		prevTime = curTime;

		memset(draw_pix, 0, w*h*4); //clear screen
		if(!pause) 
			for(int t = 0; t < tim_scale; t++)
				field_step();

		if(zoom < 2) //treated differently mostly for convenience
			draw_field(draw_pix, w, h, zoom, 0, 0);
		else
			draw_field(draw_pix, w, h, zoom, kdx, kdy);
		
		SDL_RenderClear(renderer);
		SDL_UpdateTexture(scrt, NULL, draw_pix, w * 4);
		SDL_RenderCopy(renderer, scrt, NULL, NULL);
		
		char outstr[128];
		SDL_Rect mpos;

		int curY = 25, curDY = 15;

		static float fps = 0;

		float mDT = 1000000.0 / (float)dT;
		fps = fps*0.9 + 0.1*mDT;
		sprintf(outstr, "fps %g", fps);
		//output of FPS, not necessary
/*		msg = TTF_RenderText_Solid(font, outstr, textColor); 
		mpos.x = 5; mpos.y = curY; curY += curDY;
		mpos.w = msg->w; mpos.h = msg->h;
		SDL_Texture *txt;
		txt = SDL_CreateTextureFromSurface(renderer, msg);	
		SDL_RenderCopy(renderer, txt, NULL, &mpos);
		SDL_FreeSurface(msg);
		SDL_DestroyTexture(txt);*/

		SDL_RenderPresent(renderer);
	}
	delete draw_pix;
	TTF_CloseFont( font ); 
	TTF_Quit();
	SDL_Quit();

	return 0;
}
