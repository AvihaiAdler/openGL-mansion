#include <GL\glew.h>
#include <GL\freeglut.h>
#include <math.h>
#include <time.h>
#include <stdio.h>

typedef struct Water_drop
{
	double x, y, z;
	double xt = 0, yt = 0, zt = 0;
	double theta, phi;
	double velocity;
	double acceleration = 9.8;
	double time = 0;
	bool show = true;
}Water_drop;

typedef struct House_coord 
{
	int z = -15;
	int x = 10;
	int z_height = 15;
	int x_width = 15;
}House_coord;

typedef struct Branch
{
	double length;
	int num_of_verteces;
}Branch;

typedef struct Tree
{
	double x, y, z;
	double height;
	int num_of_branches;
	Branch* branches;
}Tree;

enum tex
{
	null, wall, white, door, window, roof, drop, bush, floor_tex, sky, straight_path, curved_path, bark, red
}tex;

const double PI = 4 * atan(1.0);
const int W = 600;
const int H = 600;
const int GSZ = 100;
double ground[GSZ][GSZ] = { 0 };

double eyex = 2, eyey = 20, eyez = 30; // eye coordinates
double dir[3] = { 0,0,-1 }; // x,y,z
double speed = 0;
double angular_speed = 0;
double sight_angle = PI;
double offset = 0;

const int TEXTURE_WIDTH = 512;
const int TEXTURE_HEIGHT = 512;

unsigned char* image;
unsigned char* tmp;
unsigned char texture[TEXTURE_HEIGHT][TEXTURE_WIDTH][3];
unsigned char sky_texture[TEXTURE_HEIGHT*4][TEXTURE_WIDTH*2][3];

const int DROPS_AMOUNT = 40;
const int NUM_OF_TREES = 16;
Tree trees[NUM_OF_TREES];
Water_drop drops[DROPS_AMOUNT];
House_coord mansion;

void UpdateGround1()
{
	double delta = 0.05;
	int x1, y1, x2, y2;
	double a, b;

	if (rand() % 2 == 0)
		delta *= -1;

	x1 = rand() % GSZ;
	y1 = rand() % GSZ;
	x2 = rand() % GSZ;
	y2 = rand() % GSZ;

	if (x1 != x2)
	{
		a = ((double)y2 - y1) / (x2 - x1); // slope
		b = y1 - a*x1;
		for (int i = 0; i<GSZ; i++)
			for (int j = 0; j < GSZ; j++)
			{
				if (i > a*j + b)
					ground[i][j] += delta;
				else
					ground[i][j] -= delta;
			}
	}
}

void UpdateGround2()
{
	int num_pts = 1500;
	double delta = 0.05;
	int x, y;

	if (rand() % 2 == 0)
		delta *= -1;

	// starting point
	x = rand() % GSZ;
	y = rand() % GSZ;

	while (num_pts > 0)
	{
		num_pts--;
		ground[y][x] += delta;
		// choose next point
		switch (rand() % 4)
		{
		case 0: // go up
			y++;
			break;
		case 1: // go down
			y--;
			break;
		case 2: // go left
			x--;
			break;
		case 3: // go right
			x++;
			break;
		}
		// check for matrix borders
		x = (x + GSZ) % GSZ;
		y = (y + GSZ) % GSZ;
	}


}

void Smooth()
{
	double tmp[GSZ][GSZ];
	int i, j;

	for (i = 1; i < GSZ - 1; i++)
		for (j = 1; j < GSZ - 1; j++)
			tmp[i][j] = (0.5*ground[i - 1][j - 1] + ground[i - 1][j] + 0.5*ground[i - 1][j + 1] +
				ground[i][j - 1] + 4 * ground[i][j] + ground[i][j + 1] +
				0.5*ground[i + 1][j - 1] + ground[i + 1][j] + 0.5*ground[i + 1][j + 1]) / 10.0;

	for (i = 1; i < GSZ - 1; i++)
		for (j = 1; j < GSZ - 1; j++)
			ground[i][j] = tmp[i][j];

}

void SetColor(double h, double* colors) {
	double g = fabs(h / 6);
	if (colors == NULL)
	{
		// sand
		if (fabs(g) < 0.1)
			glColor3d(0.9, 0.8, 0.7); // biege
		else // grass
			if (fabs(g) < 0.7)
				glColor3d(g / 2, 0.8 - g, 0);
			else // snow
				glColor3d(0.7*g, 0.7*g, 0.8*g);
	}
	else
	{
		if (fabs(g) < 0.1)
		{
			*colors = 0.9; 
			*(colors + 1) = 0.8; 
			*(colors + 2) = 0.7;
		}			
		else // grass
		{
			if (fabs(g) < 0.7) 
			{
				*colors = g / 2; 
				*(colors + 1) = 0.8 - g; 
				*(colors + 2) = 0;
			}				
			else // snow
			{
				*colors = 0.7*g; 
				*(colors + 1) = 0.7*g; 
				*(colors + 2) = 0.8*g;
			}				
		}			
	}
}

//Level area
void prepareArea(double x, double z, double w, double h)
{
	//Set y level of the whole area as its mid point
	double level = ground[(int)(z + GSZ / 2 + h / 2)][(int)(x + GSZ / 2 + w / 2)];
	if (level < 0)
		level = 1;

	for (int i = (int)z + GSZ / 2; i <= z + GSZ / 2 + h; i++)
	{
		for (int j = (int)x + GSZ / 2; j <= x + GSZ / 2 + w; j++)
		{
			ground[i][j] = level;
			SetColor(ground[i][j], NULL);
		}
	}
}

//init values of the water drops
Water_drop initDrop(Water_drop drop, int i)
{
	double theta = i*30, phi = 80;	//theta -> x,z plain , phi -> x,y plain 
	double v = rand() % 6;			
	double alpha = rand() % 10;

	drop.theta = theta * PI / 180;
	drop.phi = (phi+alpha) * PI / 180;
	drop.velocity = v <= 1 ? v += 2.5 : v;

	return drop;
}

//init values of branches
Branch initBranch(Branch branch)
{
	branch.length = (rand() % (int)(1 - 0.4 + 1)) + 0.4;	//between 0.4-1	
	branch.num_of_verteces = (rand() % (10 - 4 + 1)) + 4;	//between 4-10	

	return branch;
}

//init values of trees
Tree initTree(Tree tree, double x, double y, double z)
{
	//Positions
	tree.x = x;
	tree.y = y;
	tree.z = z;
	tree.height = (rand() % 2) + 1;
	tree.num_of_branches = (rand() % (6 - 3 + 1)) + 3;

	//Init branches
	tree.branches = (Branch*)malloc(tree.num_of_branches * sizeof(Branch));
	for (int i = 0; i < tree.num_of_branches; i++)
		tree.branches[i] = initBranch(tree.branches[i]);	

	return tree;
}

void readTexture(char* fname)
{
	FILE* pf;
	fopen_s(&pf, fname, "rb");

	BITMAPFILEHEADER bf;
	BITMAPINFOHEADER bi;

	fread(&bf, sizeof(bf), 1, pf);
	fread(&bi, sizeof(bi), 1, pf);

	image = (unsigned char*)malloc(bi.biWidth*bi.biHeight * 3);
	//copy all pixels
	fread(image, 1, bi.biWidth*bi.biHeight * 3, pf);
}

void setTexture(int textureNum)
{
	int i, j, k;
	int rnd; 
	double dist, radius;
	switch (textureNum)
	{
	case 0:
		break;
	case 1:	//Stone wall
		k = 0;
		readTexture("textures/stone_wall.tex");
		for (i = 0; i < TEXTURE_HEIGHT; i++)
		{
			for (j = 0; j < TEXTURE_WIDTH; j++, k += 3)
			{
				texture[i][j][0] = image[k + 2];
				texture[i][j][1] = image[k + 1];
				texture[i][j][2] = image[k];
			}
		}
		free(image);
		break;
	case 2:	//fence (white)
		tmp = (unsigned char*)malloc(TEXTURE_HEIGHT*TEXTURE_WIDTH*3);
		for (i = 0; i < TEXTURE_HEIGHT*TEXTURE_WIDTH*3; i++)
		{
			rnd = (rand() % 30) - 15;
			*(tmp + i) = 200 + rnd;
		}
		break;
	case 3:	//Wall with door
		setTexture(1);
		k = 0;
		readTexture("textures/door.tex");
		for (i = TEXTURE_HEIGHT /4; i < TEXTURE_HEIGHT/2 + TEXTURE_HEIGHT/4; i++)
		{
			for (j = TEXTURE_WIDTH/2 - TEXTURE_WIDTH / 8 ; j < TEXTURE_WIDTH / 2 + TEXTURE_WIDTH / 8; j++, k += 3)
			{
				texture[i][j][0] = image[k + 2];
				texture[i][j][1] = image[k + 1];
				texture[i][j][2] = image[k];
			}
		}
		free(image);
		break;
	case 4:	//Wall with window
		setTexture(1);
		k = 0;
		readTexture("textures/window.tex");
		for (i = TEXTURE_HEIGHT/2 - TEXTURE_HEIGHT / 4; i < TEXTURE_HEIGHT / 2 + TEXTURE_HEIGHT / 4; i++)
		{
			for (j = TEXTURE_WIDTH / 2 - TEXTURE_WIDTH / 4; j < TEXTURE_WIDTH / 2 + TEXTURE_WIDTH / 4; j++, k += 3)
			{
				texture[i][j][0] = image[k + 2];
				texture[i][j][1] = image[k + 1];
				texture[i][j][2] = image[k];
			}
		}
		free(image);
		break;
	case 5:	//Roof
		k = 0;
		readTexture("textures/roof.tex");
		for (i = 0; i < TEXTURE_HEIGHT; i++)
		{
			for (j = 0; j < TEXTURE_WIDTH; j++, k+=3)
			{
				texture[i][j][0] = image[k + 2];
				texture[i][j][1] = image[k + 1];
				texture[i][j][2] = image[k];
			}
		}
		free(image);
		break;
	case 6:	//Water drop
		for (i = 0; i < TEXTURE_HEIGHT; i++)
		{
			for (j = 0; j < TEXTURE_WIDTH; j++)
			{
				texture[i][j][0] = 173;
				texture[i][j][1] = 216;
				texture[i][j][2] = 230;
			}
		}

		break;
	case 7:	//Bush
		k = 0;
		readTexture("textures/bush.tex");
		for (i = 0; i < TEXTURE_HEIGHT; i++)
		{
			for (j = 0; j < TEXTURE_WIDTH; j++, k += 3)
			{
				texture[i][j][0] = image[k + 2];
				texture[i][j][1] = image[k + 1];
				texture[i][j][2] = image[k];
			}
		}
		free(image);
		break;
	case 8:	//Floor
		k = 0;
		readTexture("textures/floor.tex");
		for (i = 0; i < TEXTURE_HEIGHT; i++)
		{
			for (j = 0; j < TEXTURE_WIDTH; j++, k += 3)
			{
				texture[i][j][0] = image[k + 2];
				texture[i][j][1] = image[k + 1];
				texture[i][j][2] = image[k];
			}
		}
		free(image);
		break;
	case 9:	//Sky
		k = 0;
		readTexture("textures/sky.tex");
		for (i = 0; i < TEXTURE_HEIGHT*4; i++)
		{
			for (j = 0; j < TEXTURE_WIDTH*2; j++, k += 3)
			{
				sky_texture[i][j][0] = image[k + 2];
				sky_texture[i][j][1] = image[k + 1];
				sky_texture[i][j][2] = image[k];
			}
		}
		free(image);
		break;
	case 10:	//full path (gray)
		tmp = (unsigned char*)malloc(TEXTURE_HEIGHT*TEXTURE_WIDTH * 3);
		for (i = 0; i < TEXTURE_HEIGHT*TEXTURE_WIDTH * 3; i++)
		{
			rnd = (rand() % 30) - 15;
			*(tmp + i) = 150 + rnd;
		}
		break;
	case 11:	//curved path (gray)
		radius = (TEXTURE_HEIGHT + TEXTURE_WIDTH) / 2; 
		for (i = 0; i < TEXTURE_HEIGHT; i++)
		{
			for (j = 0; j < TEXTURE_WIDTH; j++)
			{
				dist = sqrt(j*j + i*i);
				if (dist <= radius)
				{
					rnd = (rand() % 30) - 15;
					texture[i][j][0] = 150 + rnd;
					texture[i][j][1] = 150 + rnd;
					texture[i][j][2] = 150 + rnd;
				}
				else
				{
					double* tmp1 = (double*)malloc(3 * sizeof(double));
					SetColor(ground[GSZ / 2 + mansion.z + mansion.z_height/2][GSZ / 2 + mansion.x+mansion.x_width/2], tmp1);
					texture[i][j][0] = *tmp1*255;
					texture[i][j][1] = *(tmp1+1)*255;
					texture[i][j][2] = *(tmp1+2)*255;
					free(tmp1);
				}				
			}
		}
		break;
	case 12:		//bark
		k = 0;
		readTexture("textures/bark.tex");
		for (i = 0; i < TEXTURE_HEIGHT; i++)
		{
			for (j = 0; j < TEXTURE_WIDTH; j++, k += 3)
			{
				texture[i][j][0] = image[k + 2];
				texture[i][j][1] = image[k + 1];
				texture[i][j][2] = image[k];
			}
		}
		free(image);
		break;
	case 13:		//mailBox (red)
		k = 0;
		for (i = 0; i < TEXTURE_HEIGHT; i++)
		{
			for (j = 0; j < TEXTURE_WIDTH; j++, k += 3)
			{
				texture[i][j][0] = 255*0.6;
				texture[i][j][1] = 0;
				texture[i][j][2] = 0;
			}
		}
		break;
	}
}

void initTextures()
{
	setTexture(1);
	glBindTexture(GL_TEXTURE_2D, 1);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, TEXTURE_HEIGHT, TEXTURE_WIDTH, 0, GL_RGB, GL_UNSIGNED_BYTE, texture);

	setTexture(2);
	glBindTexture(GL_TEXTURE_2D, 2);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, TEXTURE_HEIGHT, TEXTURE_WIDTH, 0, GL_RGB, GL_UNSIGNED_BYTE, tmp);
	free(tmp);

	setTexture(3);
	glBindTexture(GL_TEXTURE_2D, 3);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, TEXTURE_HEIGHT, TEXTURE_WIDTH, 0, GL_RGB, GL_UNSIGNED_BYTE, texture);

	setTexture(4);
	glBindTexture(GL_TEXTURE_2D, 4);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, TEXTURE_HEIGHT, TEXTURE_WIDTH, 0, GL_RGB, GL_UNSIGNED_BYTE, texture);

	setTexture(5);
	glBindTexture(GL_TEXTURE_2D, 5);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, TEXTURE_HEIGHT, TEXTURE_WIDTH, 0, GL_RGB, GL_UNSIGNED_BYTE, texture);

	setTexture(6);
	glBindTexture(GL_TEXTURE_2D, 6);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, TEXTURE_HEIGHT, TEXTURE_WIDTH, 0, GL_RGB, GL_UNSIGNED_BYTE, texture);

	setTexture(7);
	glBindTexture(GL_TEXTURE_2D, 7);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, TEXTURE_HEIGHT, TEXTURE_WIDTH, 0, GL_RGB, GL_UNSIGNED_BYTE, texture);

	setTexture(8);
	glBindTexture(GL_TEXTURE_2D, 8);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, TEXTURE_HEIGHT, TEXTURE_WIDTH, 0, GL_RGB, GL_UNSIGNED_BYTE, texture);

	setTexture(9);
	glBindTexture(GL_TEXTURE_2D, 9);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, TEXTURE_HEIGHT*4, TEXTURE_WIDTH*2, 0, GL_RGB, GL_UNSIGNED_BYTE, sky_texture);

	setTexture(10);
	glBindTexture(GL_TEXTURE_2D, 10);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, TEXTURE_HEIGHT, TEXTURE_WIDTH, 0, GL_RGB, GL_UNSIGNED_BYTE, tmp);
	free(tmp);

	setTexture(11);
	glBindTexture(GL_TEXTURE_2D, 11);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, TEXTURE_HEIGHT, TEXTURE_WIDTH, 0, GL_RGB, GL_UNSIGNED_BYTE, texture);

	setTexture(12);
	glBindTexture(GL_TEXTURE_2D, 12);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, TEXTURE_HEIGHT, TEXTURE_WIDTH, 0, GL_RGB, GL_UNSIGNED_BYTE, texture);

	setTexture(13);
	glBindTexture(GL_TEXTURE_2D, 13);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, TEXTURE_HEIGHT, TEXTURE_WIDTH, 0, GL_RGB, GL_UNSIGNED_BYTE, texture);
}

void init()
{
	int i;
	double x = 0, z = 0;
	glEnable(GL_DEPTH_TEST);
	srand((unsigned int)time(0));

	glClearColor(0, 0, 0.5, 0);
	glEnable(GL_NORMALIZE);

	for (i = 1; i < 1000; i++)
		UpdateGround1();
	for (i = 1; i < 1000; i++)
		UpdateGround2();
	Smooth();

	//Level the area
	prepareArea(mansion.x - 1, mansion.z - 1, mansion.x_width + 2, mansion.z_height + 2);

	//Init water drops	
	for (i = 0; i < DROPS_AMOUNT; i++)
		drops[i] = initDrop(drops[i], i);

	//init trees
	for (i = 0; i < NUM_OF_TREES; i++)
	{
		//randomize positions for trees
		do 
		{
			z = (rand() % ((mansion.z_height - 1) - 1 + 1)) + 1;
		} while (z < mansion.z_height / 2 + 3 && z > mansion.z_height / 2 - 3);

		if (z > mansion.z_height / 2 +4)
			x = (rand() % (mansion.x_width - 1 - 1 + 1)) + 1;
		else
			x = (rand() % (mansion.x_width / 2 - 1 - 1 + 1)) + 1;
		
		trees[i] = initTree(trees[i], mansion.x + x, ground[mansion.z + GSZ / 2][mansion.x + GSZ / 2], mansion.z + z);
	}	

	initTextures();
}

void DrawAxes()
{
	glLineWidth(2);
	// x:
	glColor3d(1, 0, 0);
	glBegin(GL_LINES);
	glVertex3d(0, 0, 0);
	glVertex3d(20, 0, 0);
	glEnd();
	// y:
	glColor3d(0, 1, 0);
	glBegin(GL_LINES);
	glVertex3d(0, 0, 0);
	glVertex3d(0, 20, 0);
	glEnd();
	// z:
	glColor3d(1, 0, 1);
	glBegin(GL_LINES);
	glVertex3d(0, 0, 0);
	glVertex3d(0, 0, 20);
	glEnd();

	glLineWidth(1);

}

void DrawFloor()
{
	int i, j;
	glColor3d(1, 1, 1);

	glDisable(GL_LIGHTING);
	for (i = 0; i<GSZ - 2; i++)
		for (j = 0; j < GSZ - 2; j++)
		{

			glBegin(GL_POLYGON);
			SetColor(ground[i][j],NULL);
			glVertex3d(j - GSZ / 2, ground[i][j], i - GSZ / 2);// 1-st point
			SetColor(ground[i][j + 1],NULL);
			glVertex3d(j - GSZ / 2 + 1, ground[i][j + 1], i - GSZ / 2);// 2-nd point
			SetColor(ground[i + 1][j + 1],NULL);
			glVertex3d(j - GSZ / 2 + 1, ground[i + 1][j + 1], i - GSZ / 2 + 1);// 3-d point
			SetColor(ground[i + 1][j],NULL);
			glVertex3d(j - GSZ / 2, ground[i + 1][j], i - GSZ / 2 + 1);// 3-d point
			glEnd();
		}

	// transparent water
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glColor4d(0, 0.4, 0.6, 0.8);
	glBegin(GL_POLYGON);
	glVertex3d(-GSZ / 2, 0, -GSZ / 2);
	glVertex3d(GSZ / 2, 0, -GSZ / 2);
	glVertex3d(GSZ / 2, 0, GSZ / 2);
	glVertex3d(-GSZ / 2, 0, GSZ / 2);
	glEnd();
	glDisable(GL_BLEND);

	glEnable(GL_LIGHTING);

}

//Draw a circle - must use glDisable(GL_LIGHTING)
void DrawCircle(int sides, double radius, int part)
{
	if (part > 0)
	{
		glBegin(GL_POLYGON);
		glVertex3d(0, 0, 0);
		for (int i = 0; i <= sides / part; i++)
		{
			glVertex3d(radius*sin(i * 2 * PI / sides), 0, radius*cos(i * 2 * PI / sides));
		}
		glEnd();
	}	
}

//Draw a prism at the origin with a height of 1
void DrawCylinderWithTexture(double starting_angle, int sides, int textureNum, double topr, double bottomr, int hrepeat, double vstart, double vend)	
{
	double part = (double)hrepeat / sides;	//part of a texture that falls on one side of the cylinder
	int counter;
	double alpha, teta = 2 * PI / sides;

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, textureNum);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

	for (alpha = starting_angle, counter = 0; alpha <= 2 * PI; alpha += teta, counter++)
	{
		glBegin(GL_POLYGON);
		glTexCoord2d(counter*part, vend);				glVertex3d(topr*sin(alpha), 1, topr*cos(alpha)); // point 2
		glTexCoord2d((counter + 1)*part, vend);			glVertex3d(topr*sin(alpha + teta), 1, topr*cos(alpha + teta)); // point 3
		glTexCoord2d((1 + counter)*part, vstart);		glVertex3d(bottomr*sin(alpha + teta), 0, bottomr*cos(alpha + teta)); // point 4
		glTexCoord2d(counter*part, vstart);				glVertex3d(bottomr*sin(alpha), 0, bottomr*cos(alpha)); // point 1
		glEnd();
	}

	glDisable(GL_TEXTURE_2D);
}

//Draw a sphere at the origin
void DrawSphereWithTexture(int slices, int sectors, int tnum, double start_angle, double end_angle, int hrepeat, int vrepeat)
{
	//Full Spehre: start_angle = -PI/2, end_angle = PI/2
	double beta, gamma = PI / slices;
	double vpart = vrepeat / (double)slices;
	int counter;

	// for each slice
	for (beta = start_angle, counter = 0; beta < end_angle; beta += gamma, counter++)
	{ // draw one cylinder
		glPushMatrix();
		glTranslated(0, sin(beta), 0);
		glScaled(1, fabs(sin(beta) - sin(beta + gamma)), 1);
		DrawCylinderWithTexture(0, sectors, tnum, cos(beta + gamma), cos(beta), hrepeat, counter*vpart, vpart*(counter + 1));
		glPopMatrix();
	}
}

//Draw a wall parallel to the x axis on the origin
void DrawSurface(double width, double height, int tnum, int hrepeat, int vrepeat)
{
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, tnum);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

	glBegin(GL_POLYGON);
	glTexCoord2d(0, 0);
	glVertex3d(-width / 2, -height/2, 0);
	glTexCoord2d(0, hrepeat);
	glVertex3d(-width / 2, height/2, 0);
	glTexCoord2d(vrepeat, hrepeat);
	glVertex3d(width / 2, height/2, 0);
	glTexCoord2d(vrepeat, 0);
	glVertex3d(width/2, -height/2, 0);
	glEnd();

	glDisable(GL_TEXTURE_2D);
}

//Draw a cube at the origin where the origin is it's mid-point
void DrawCubeWithTexture(double width, double height, int sides_x_tex, int sides_z_tex, int cover_tex, int hrepeat, int vrepeat)
{
	double angle = 0;
	int factor = 1, tex = 0;
	glPushMatrix();
	glTranslated(0, height/2, 0);
	for (int i = 0; i < 4; i++, angle += 90)
	{
		glPushMatrix();
		if (i % 2 == 0)
		{
			glTranslated(0, 0, factor*width / 2);
			factor *= -1;
			tex = sides_z_tex;
		}
		else
		{
			glTranslated(factor*width / 2, 0, 0);
			tex = sides_x_tex;
		}
		glRotated(angle, 0, 1, 0);
		DrawSurface(width, height, tex, 1, 1);
		glPopMatrix();
	}	

	for (int i = 0; i < 2; i++)
	{
		glPushMatrix();
		if (i % 2 != 0)
			factor *= -1;
		glTranslated(0, factor*height/2, 0);
		glRotated(90, 1, 0, 0);
		DrawSurface(width, height, cover_tex, 1, 1);
		glPopMatrix();
	}
	glPopMatrix();
}

//Draw a triangle at the origin where the origin is it's mid-point
void DrawTriangleWithTexture(double width, double height, int tnum, int hrepeat, int vrepeat)
{
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, tnum);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

	//Far
	glBegin(GL_POLYGON);
	glTexCoord2d(0, 0);					glVertex3d(0 - width / 2, 0, 0 - width / 2);
	glTexCoord2d(0, hrepeat);			glVertex3d(0 - width / 2, height, 0);
	glTexCoord2d(vrepeat, hrepeat);		glVertex3d(width / 2, height, 0);
	glTexCoord2d(vrepeat, 0);			glVertex3d(width / 2, 0, 0 - width / 2);
	glEnd();

	//Close
	glBegin(GL_POLYGON);
	glTexCoord2d(0, 0);					glVertex3d(0 - width / 2, 0, width / 2);
	glTexCoord2d(0, hrepeat);			glVertex3d(0 - width / 2, height, 0);
	glTexCoord2d(vrepeat, hrepeat);		glVertex3d(width / 2, height, 0);
	glTexCoord2d(vrepeat, 0);			glVertex3d(width / 2, 0, width / 2);
	glEnd();

	//Right
	glBegin(GL_POLYGON);
	glTexCoord2d(0, 0);					glVertex3d(width / 2, 0, 0 - width / 2);
	glTexCoord2d(0, hrepeat);			glVertex3d(width / 2, height, 0);
	glTexCoord2d(vrepeat, 0);			glVertex3d(width / 2, 0, width / 2);
	glEnd();

	//Left
	glBegin(GL_POLYGON);
	glTexCoord2d(0, 0);					glVertex3d(0 - width / 2, 0, 0 - width / 2);
	glTexCoord2d(0, hrepeat);			glVertex3d(0 - width / 2, height, 0);
	glTexCoord2d(vrepeat, 0);			glVertex3d(0 - width / 2, 0, width / 2);
	glEnd();

	glDisable(GL_TEXTURE_2D);
}

void drawBush(double x, double y, double z, int texture)
{
	glPushMatrix();
	glTranslated(x, y, z);
	glScaled(0.15, 0.3, 0.15);
	DrawSphereWithTexture(30, 30, texture, 0, PI / 2, 1, 1);
	glPopMatrix();
}

void drawBranch(Branch branch)
{
	glDisable(GL_LIGHTING);

	double alpha, beta;

	beta = PI / branch.num_of_verteces;

	glPushMatrix();
	glTranslated(-branch.length, 0, 0);
	glPushMatrix();
	glLineWidth(2.2);
	glColor3d(0, 0.35, 0);
	glBegin(GL_LINE_STRIP);
	for (alpha = 0; alpha < PI; alpha += beta)
		glVertex3d(branch.length*cos(alpha), branch.length / 1.5*sin(alpha), 0);

	glEnd();
	glPopMatrix();
	glPopMatrix();


	glEnable(GL_LIGHTING);
}
 
void drawTree(Tree tree)
{
	glDisable(GL_LIGHTING);

	int i;
	double alpha, beta;

	beta = (double)360 / tree.num_of_branches;

	//Trunk
	glPushMatrix();
	glTranslated(tree.x, tree.y, tree.z);
	glScaled(0.2, tree.height, 0.2);
	DrawCylinderWithTexture(0, 30, 12, 0.5, 0.5, tree.height, 0, tree.height);
	glPopMatrix();

	//top cover
	glPushMatrix();
	glTranslated(tree.x, tree.y + tree.height, tree.z);
	glScaled(0.2, 0.02, 0.2);
	DrawCylinderWithTexture(0, 30, 12, 0, 0.5, 1, 0, 1);
	glPopMatrix();


	//Branches
	for (i = 0, alpha = 0; i < tree.num_of_branches; i++, alpha += beta)
	{
		glPushMatrix();
		glTranslated(tree.x, (tree.y + tree.height)*0.9, tree.z);
		glRotated(alpha, 0, 1, 0);
		drawBranch(tree.branches[i]);
		glPopMatrix();
	}

	glEnable(GL_LIGHTING);
}

void drawMailbox()
{
	double postHeight = 2;

	//post
	glPushMatrix();
	glScaled(0.15, postHeight, 0.15);
	DrawCylinderWithTexture(0, 30, 2, 0.5, 0.5, 1, 0, 1);
	glPopMatrix();

	//box
	glPushMatrix();
	glTranslated(0, postHeight, 0);
	glScaled(1.2, 0.5, 0.5);
	DrawCubeWithTexture(1, 1, 13, 13, 13, 1, 1);
	glPopMatrix();

	//round part of the box
	glPushMatrix();
	glTranslated(- 1.2/2, postHeight + 0.5, 0);
	glRotated(-90, 0, 0, 1);
	glScaled(0.5, 1.2, 0.5);
	DrawCylinderWithTexture(PI, 30, 13, 0.5, 0.5, 1, 0, 1);
	glPopMatrix();

	glDisable(GL_LIGHTING);
	for (int i = 0, j=-1; i < 2; i++, j*=-1)
	{
		glPushMatrix();
		glTranslated((0.4999*1.2)*j,  postHeight + 0.5, 0);
		glRotated(90, 0, 0, 1);
		glColor3d(0.6, 0, 0);
		DrawCircle(30, 0.25, 1);
		glPopMatrix();
	}
	glEnable(GL_LIGHTING);
	
}

//Draw the entire yard
void drawYard(int left, int away, int width, int height)
{
	double ratio = 8;
	double fenceHeight = 0.5;

	//Bushes
	for (int j = away + height / 2 - 2; j <= away + height / 2; j++)
	{
		if (j != away + height / 2 - 1)
		{
			for (int i = left + 1; i < left + width - 2.5*(width / ratio) + 1; i++)
			{
				if (i > left + width / 2 + 2 || i <= left + width / 2 - 1)
				{
					glPushMatrix();
					drawBush(i, ground[away + GSZ / 2][left + GSZ / 2], j, 7);
					glPopMatrix();
				}				
			}				
		}		
	}

	//mailbox
	glPushMatrix();
	glTranslated(left, ground[away + GSZ / 2][left + GSZ / 2], away + height / 2 - 0.3);
	glScaled(0.25, 0.25, 0.25);
	drawMailbox();
	glPopMatrix();

	//trees
	for (int i = 0; i < NUM_OF_TREES; i++)
	{
		glPushMatrix();
		drawTree(trees[i]);
		glPopMatrix();
	}
		

	//Paths
	for (int i = left + 1; i < left + width -1.5*(width/ratio) + 1; i++)
	{
		if (i != left + width / 2 + 1)
		{
			glPushMatrix();
			glTranslated(i, ground[away + GSZ / 2][left + GSZ / 2] + 0.001, away + height / 2 - 1);
			glRotated(90, 1, 0, 0);
			DrawSurface(1, 1, 10, 1, 1);
			glPopMatrix();
		}
		else
		{
			for (int j = 0; j < 2; j++)
			{
				glPushMatrix();
				glTranslated(i, ground[away + GSZ / 2][left + GSZ / 2] + 0.001, away + height / 2 + j*-2);
				glRotated(90, 1, 0, 0);
				DrawSurface(1, 1, 10, 1, 1);
				glPopMatrix();
			}
		}		
	}

	//Paths (circling the fountain)
	for (int i = 1; i <= 4; i++)
	{
		double angle = 90;
		double z = away + height / 2 - 2;
		double x = left + width / 2 + 2;
		
		if (i < 3)
		{
			x = left + width / 2;
			if (i % 2 == 1)
			{
				z = away + height / 2;
			}
		}
		else
		{
			x = left + width / 2 + 2;
			if (i % 2 == 0)
			{
				z = away + height / 2;
			}
		}
		glPushMatrix();
		glTranslated(x, ground[away + GSZ / 2][left + GSZ / 2] + 0.001, z);
		glRotated(angle*i, 0, -1, 0);
		glRotated(90, 1, 0, 0);
		DrawSurface(1, 1, 11, 1, 1);
		glPopMatrix();
	}


	

	//Fence:
	//Far side
	glPushMatrix();
	glTranslated(left, ground[away + GSZ / 2][left + GSZ / 2] + fenceHeight/2, away);
	glRotated(90, 0, 0, 1);
	glScaled(0.1, -width, 0.1);	
	DrawCylinderWithTexture(0, 40, 2, 0.5, 0.5, 1, 0, 1);
	glPopMatrix();

	for (double i = left; i < left + width; i+=0.5)
	{
		glPushMatrix();
		glTranslated(i, ground[away + GSZ / 2][(int)i + GSZ / 2] + fenceHeight, away);
		glScaled(0.2, 0.2, 0.2);
		DrawCylinderWithTexture(0, 40, 2, 0, 0.5, 1, 0, 1);
		glPopMatrix();

		glPushMatrix();
		glTranslated(i, ground[away + GSZ / 2][(int)i + GSZ / 2], away);
		glScaled(0.2, fenceHeight, 0.2);
		DrawCylinderWithTexture(0, 40, 2, 0.5, 0.5, 1, 0, 1);
		glPopMatrix();
	}

	//Near side
	glPushMatrix();
	glTranslated(left, ground[away + height + GSZ / 2][left + GSZ / 2] + fenceHeight/2, away + height);
	glRotated(90, 0, 0, 1);
	glScaled(0.1, -width, 0.1);
	DrawCylinderWithTexture(0, 40, 2, 0.5, 0.5, 1, 0, 1);
	glPopMatrix();

	for (double i = left; i < left + width; i+=0.5)
	{
		glPushMatrix();
		glTranslated(i, ground[away + height + GSZ / 2][(int)i + GSZ / 2] + fenceHeight, away + height);
		glScaled(0.2, 0.2, 0.2);
		DrawCylinderWithTexture(0, 40, 2, 0, 0.5, 1, 0, 1);
		glPopMatrix();

		glPushMatrix();
		glTranslated(i, ground[away + height + GSZ / 2][(int)i + GSZ / 2], away + height);
		glScaled(0.2, fenceHeight, 0.2);
		DrawCylinderWithTexture(0, 40, 2, 0.5, 0.5, 1, 0, 1);
		glPopMatrix();
	}

	//Right side
	glPushMatrix();
	glTranslated(left + width, ground[away + height + GSZ / 2][left + width + GSZ / 2] + fenceHeight/2, away);
	glRotated(-90, 0, 1, 0);
	glRotated(90, 0, 0, 1);
	glScaled(0.1, -height, 0.1);
	DrawCylinderWithTexture(0, 40, 2, 0.5, 0.5, 1, 0, 1);
	glPopMatrix();

	for (double i = away; i < away + height; i+=0.5)
	{
		glPushMatrix();
		glTranslated(left + width, ground[(int)i + GSZ / 2][left + width + GSZ / 2] + fenceHeight, i);
		glScaled(0.2, 0.2, 0.2);
		DrawCylinderWithTexture(0, 40, 2, 0, 0.5, 1, 0, 1);
		glPopMatrix();

		glPushMatrix();
		glTranslated(left + width, ground[(int)i + GSZ / 2][left + width + GSZ / 2], i);
		glScaled(0.2, fenceHeight, 0.2);
		DrawCylinderWithTexture(0, 40, 2, 0.5, 0.5, 1, 0, 1);
		glPopMatrix();
	}

	//Left side
	glPushMatrix();
	glTranslated(left, ground[away + height + GSZ / 2][left + GSZ / 2] + fenceHeight/2, away);
	glRotated(-90, 0, 1, 0);
	glRotated(90, 0, 0, 1);
	glScaled(0.1, -(height/2 - 2), 0.1);
	DrawCylinderWithTexture(0, 40, 2, 0.5, 0.5, 1, 0, 1);
	glPopMatrix();

	glPushMatrix();
	glTranslated(left, ground[away + height + GSZ / 2][left + GSZ / 2] + fenceHeight/2, away + height);
	glRotated(90, 0, 1, 0);
	glRotated(90, 0, 0, 1);
	glScaled(0.1, -(height / 2 + 1), 0.1);
	DrawCylinderWithTexture(0, 40, 2, 0.5, 0.5, 1, 0, 1);
	glPopMatrix();

	for (double i = away; i < away + height; i+=0.5)
	{
		if (i > away+height/2-0.5 || i < away + height / 2 - 1.5)
		{
			glPushMatrix();
			glTranslated(left, ground[(int)i + GSZ / 2][left + GSZ / 2] + fenceHeight, i);
			glScaled(0.2, 0.2, 0.2);
			DrawCylinderWithTexture(0, 40, 2, 0, 0.5, 1, 0, 1);
			glPopMatrix();

			glPushMatrix();
			glTranslated(left, ground[(int)i + GSZ / 2][left + GSZ / 2], i);
			glScaled(0.2, fenceHeight, 0.2);
			DrawCylinderWithTexture(0, 40, 2, 0.5, 0.5, 1, 0, 1);
			glPopMatrix();
		}
	}
}

void drawDrop(Water_drop drop)
{
	glDisable(GL_LIGHTING);

	glPushMatrix();
	glTranslated(drop.xt, drop.yt, drop.zt);
	glScaled(0.02, 0.035, 0.02);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	DrawSphereWithTexture(30, 30, 6, -PI / 2, PI / 2, 1, 1);
	glDisable(GL_BLEND);

	glPopMatrix();

	glEnable(GL_LIGHTING);
}

Water_drop initDropLocation(Water_drop drop, double x, double y, double z)
{
	drop.x = x;
	drop.y = y;
	drop.z = z;
	return drop;
}

void drawFountain(double x, double y, double z)
{
	//Fountain base
	glPushMatrix();
	glTranslated(x, y + 0.1, z);
	glScaled(0.4, 0.4, 0.4);
	DrawSphereWithTexture(30, 30, 2, -PI / 2, 0, 1, 1);
	glPopMatrix();

	//Sprinkler
	glPushMatrix();
	glTranslated(x, y, z);
	glScaled(0.2, 0.15, 0.2);
	DrawCylinderWithTexture(0, 30, 2, 0.2, 0.3, 1, 0, 1);
	glPopMatrix();

	//Water drops
	glPushMatrix();
	glTranslated(x, y + 0.1, z);
	glDisable(GL_LIGHTING);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glColor4d(0, 0.4, 0.6, 0.8);
	DrawCircle(30, 0.4, 1);
	glDisable(GL_BLEND);
	glEnable(GL_LIGHTING);
	glPopMatrix();

	for (int i = 0; i < DROPS_AMOUNT; i++)
		drops[i] = initDropLocation(drops[i], x, y + 0.1, z);

	for (int i = 0; i < DROPS_AMOUNT; i++)
		drawDrop(drops[i]);
}

void drawHouse(int left, int away, int width, int height)
{
	int ratio = 8;
	double ground_level = ground[away + height/2 + GSZ / 2][left + width/2 + GSZ / 2];
	double house_height = width / ratio, house_width = width/ratio;

	drawYard(left, away, width, height);

	drawFountain((left + width) - 7* (width / ratio), ground_level, away + height / 2 - height / ratio);

	//Far side - house
	for (double i = left + 8*(width / ratio); i < (left + width) - 2.5*(width / ratio); i += house_width)
	{
		glPushMatrix();
		glTranslated(i, ground_level, away + 2*(height / ratio));
		DrawCubeWithTexture(house_width, house_height, wall, window, floor_tex, 1, 1);
		glPopMatrix();
	}
	//Far side - 2nd floor
	int counter = 0;
	for (double i = left + 8 * (width / ratio); i < (left + width) - 2.5*(width / ratio); i += house_width/2)
	{
		//Balcony door
		if (counter == 4)
		{
			glPushMatrix();
			glTranslated(i, ground_level + house_height, away + 1.8*(height / ratio)+0.001);
			DrawCubeWithTexture(house_width*0.5, house_height *0.5, wall, door, wall, 1, 1);
			glPopMatrix();
		}
			
		counter++;

		glPushMatrix();
		glTranslated(i, ground_level + house_height, away + 1.8*(height / ratio));
		DrawCubeWithTexture(house_width*0.5, house_height *0.5, wall, window, wall, 1, 1);
		glPopMatrix();

		//Roof
		glPushMatrix();
		glTranslated(i, ground_level + 1.5*house_height, away + 1.8 * (height / ratio));
		DrawTriangleWithTexture(house_width*0.5, house_height *0.25, roof, 1, 1);
		glPopMatrix();
	}

	//Balcony stairs
	for (double i = 0, j = 0; i < house_height / 8;i+= (house_height/8)/3, j += 0.5 / 9)
	{
		glPushMatrix();
		glTranslated(left + 8 * (width / ratio) + 4*house_width/2, ground_level + house_height + i, away + 2.1*(height / ratio) - j);
		glScaled(0.5, 0.1, 0.5);
		DrawCubeWithTexture(0.5, 0.5, wall, wall, white, 1, 1);
		glPopMatrix();
	}

	//Balcony - fence - vertical
	for (double i = left + 7.5 * (width / ratio); i < (left + width) - 2.5*(width / ratio); i += house_width / 4)
	{		
		glPushMatrix();
		glTranslated(i, ground_level + house_height, away + 2.5*(height / ratio));
		glScaled(1, 0.2, 1);
		DrawCylinderWithTexture(0, 30, white, 0.01, 0.01, 1, 0, 1);
		glPopMatrix();

		//Balcony - fence - horizontal
		if (i != left + 7.5 * (width / ratio))
		{
			glPushMatrix();
			glTranslated(i, ground_level + house_height + 0.2, away + 2.5*(height / ratio));
			glRotated(90, 0, 0, 1);
			glScaled(1, 0.25, 1);
			DrawCylinderWithTexture(0, 30, white, 0.01, 0.01, 1, 0, 1);
			glPopMatrix();
		}	
	}

	//Far side - tower
	for (double i = ground_level; i < ground_level + 1.5*house_height; i+=house_height)
	{
		double angle = PI;
		if (i >= ground_level + house_height)
			angle = 0;

		glPushMatrix();
		glTranslated(left + 7 * (width / ratio) + 0.5*(width / ratio), i, away + 2 * (height / ratio));
		DrawCylinderWithTexture(angle, 10, window, house_width/2, house_width/2, 10, 0, 1);
		glPopMatrix();
	}

	//roof
	glPushMatrix();
	glTranslated(left + 7 * (width / ratio) + 0.5*(width / ratio), ground_level + 2*house_height, away + 2 * (height / ratio));
	glScaled(1, 0.5, 1);
	DrawCylinderWithTexture(0, 10, roof, 0, house_width / 2 + 0.1, 5, 0, 1);
	glPopMatrix();

	glPushMatrix();
	glTranslated(left + 7 * (width / ratio) + 0.5*(width / ratio), ground_level + 2 * house_height, away + 2 * (height / ratio));
	glScaled(1, 0.1, 1);
	DrawCylinderWithTexture(0, 10, roof, 0, house_width / 2 + 0.1, 5, 0, 1);
	glPopMatrix();

	//Right side - house
	for (int i = away + 3 * (height / ratio); i < (away + height) - 3*(height / ratio); i += height/ ratio)
	{
		glPushMatrix();
		glTranslated((left + width) - 2*(width / ratio), ground_level, i);
		DrawCubeWithTexture(house_width, house_height, window, wall, floor_tex, 1, 1);
		glPopMatrix();

		if (i + height / ratio == (away + height) - 3 * (height / ratio))
		{
			glPushMatrix();
			glTranslated((left + width) - 2 * (width / ratio), ground_level + house_height, i);
			DrawCubeWithTexture(house_width, house_height, window, wall, wall, 1, 1);
			glPopMatrix();
		}
	}

	//Right side - 2nd floor
	counter = 0;
	for (double i = away + 3 * (height / ratio); i < (away + height) - 4 * (height / ratio); i += house_width/2)
	{
		int tex = window;
		//Balcony door
		if (counter == 4)
		{
			glPushMatrix();
			glTranslated((left + width) - 1.8 * (width / ratio) - 0.001, ground_level + house_height, i);
			DrawCubeWithTexture(house_width*0.5, house_height *0.5, door, wall, wall, 1, 1);
			glPopMatrix();
		}

		counter++;

		if (counter % 2 == 0)
			tex = wall;

		glPushMatrix();
		glTranslated((left + width) - 1.8 * (width / ratio), ground_level + house_height, i);
		DrawCubeWithTexture(house_width*0.5, house_height *0.5, tex, wall, wall, 1, 1);
		glPopMatrix();

		//Roof
		glPushMatrix();
		glTranslated((left + width) - 1.8 * (width / ratio), ground_level + 1.5*house_height, i);
		glRotated(90, 0, 1, 0);
		DrawTriangleWithTexture(house_width*0.5, house_height *0.25, roof, 1, 1);
		glPopMatrix();
	}

	//Balcony - stairs
	for (double i = 0, j = 0; i < house_height / 8;i+= (house_height/8)/3, j += 0.5 / 9)
	{
		glPushMatrix();
		glTranslated((left + width) - 2.1 * (width / ratio) + j, ground_level + house_height + i,  away + 3 * (height / ratio) + 4*house_width/2);
		glScaled(0.5, 0.1, 0.5);
		DrawCubeWithTexture(0.5, 0.5, wall, wall, white, 1, 1);
		glPopMatrix();
	}

	//Balcony - fence - vertical
	for (double i = away + 3 * (height / ratio); i <= (away + height) - 4.5 * (height / ratio); i += house_width / 4)
	{
		glPushMatrix();
		glTranslated((left + width) - 2.5 * (width / ratio), ground_level + house_height, i);
		glScaled(1, 0.2, 1);
		DrawCylinderWithTexture(0, 30, white, 0.01, 0.01, 1, 0, 1);
		glPopMatrix();

		//Balcony - fence - horizontal
		if (i != away + 3 * (height / ratio) && i != (away + height) - 4.5 * (height / ratio))
		{
			glPushMatrix();
			glTranslated((left + width) - 2.5 * (width / ratio), ground_level + house_height + 0.2, i);
			glRotated(90, 1, 0, 0);
			glScaled(1, 0.25, 1);
			DrawCylinderWithTexture(0, 30, white, 0.01, 0.01, 1, 0, 1);
			glPopMatrix();
		}
	}

	//Middle - entrance
	glPushMatrix();
	glTranslated((left + width) - 2* (width / ratio) - 0.001, ground_level, away + height / 2 - height/ratio);
	DrawCubeWithTexture(house_width, house_height, door, wall, floor_tex, 1, 1);
	glPopMatrix();

	//Stairs - entrance
	for (double i = 0, j = 0.5; i < house_height / 4; i+= (house_height / 4)/3, j-= 0.5/3)
	{
		glPushMatrix();
		glTranslated((left + width) - 2* (width / ratio) - j, ground_level + i, away + height / 2 - height / ratio);
		glScaled(1, 0.1, 1);
		DrawCylinderWithTexture(PI, 30, wall, 0.5, 0.5, 1, 0, 1);
		glPopMatrix();

		glDisable(GL_LIGHTING);
		glPushMatrix();
		glTranslated((left + width) - 2* (width / ratio) - j, ground_level + i + (house_height / 4) / 3, away + height / 2 - height / ratio);
		glColor3d(0.8, 0.8, 0.8);
		DrawCircle(30, 0.5, 1);
		glPopMatrix();
		glEnable(GL_LIGHTING);
	}
	
	//Middle - tower
	for (double i = ground_level; i < ground_level + 1.5*house_height; i++)
	{
		int j = wall;
		if (i > ground_level)
			j = window;
		glPushMatrix();
		glTranslated((left + width) - 2.5*(width / ratio), i, away + 2.5 * (height / ratio));
		DrawCylinderWithTexture(0, 12, j, house_width, house_width, 12, 0, 1);
		glPopMatrix();
	}

	//Roof
	glPushMatrix();
	glTranslated((left + width) - 2.5*(width / ratio), ground_level + 2*house_height, away + 2.5 * (height / ratio));
	glScaled(1, 0.1, 1);
	DrawCylinderWithTexture(0, 12, roof, 0, house_width+0.2, 6, 0, 1);
	glPopMatrix();

	glPushMatrix();
	glTranslated((left + width) - 2.5*(width / ratio), ground_level + 2 * house_height, away + 2.5 * (height / ratio));
	DrawCylinderWithTexture(0, 12, roof, 0, house_width+0.2, 6, 0, 1);
	glPopMatrix();

}

void display()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, W, H);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glFrustum(-1, 1, -1, 1, 0.7, 300);
	// camera location and direction
	gluLookAt(eyex, eyey, eyez, // EYE coordinates
		eyex + dir[0], eyey - 0.2, eyez + dir[2], // Point Of Interest
		0, 1, 0); // UP

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	DrawAxes();
	DrawFloor();
	drawHouse(mansion.x, mansion.z, mansion.x_width, mansion.z_height);

	//Sky
	glPushMatrix();
	glRotated(offset, 0, 1, 0);
	glTranslated(0, 10, 0);
	glScaled(110, 110, 110);
	DrawSphereWithTexture(240, 300, 9, -PI / 2, PI / 2, 1, 1);
	glPopMatrix();

	glDisable(GL_LIGHTING);

	glutSwapBuffers();
}

void idle()
{
	offset += 0.04;
	//ego-motion
	dir[0] = sin(sight_angle); // x
	dir[2] = cos(sight_angle); //z

	//Water drops - position calculation
	for (int i = 0; i < DROPS_AMOUNT; i++) 
	{
		if (drops[i].yt <= ground[(int)drops[i].zt + GSZ / 2][(int)drops[i].xt + GSZ / 2])
		{
			drops[i].show = false;
			drops[i].xt = drops[i].x;
			drops[i].yt = drops[i].y;
			drops[i].zt = drops[i].z;
			drops[i].time = 0;
		}
		else
		{
			drops[i].show = true;
		}

		if (drops[i].show)
		{
			drops[i].time += 0.005;

			//Calculate drop position
			drops[i].xt = drops[i].x + drops[i].velocity*cos(drops[i].theta)*cos(drops[i].phi)*drops[i].time;	//x
			drops[i].yt = drops[i].y + drops[i].velocity*sin(drops[i].phi)*drops[i].time - 0.5*drops[i].acceleration*drops[i].time*drops[i].time;	//y
			drops[i].zt = drops[i].z + drops[i].velocity*sin(drops[i].theta)*cos(drops[i].phi)*drops[i].time;	//z
			for (int j = 0; j < 512; j++);	//Delay
		}
	}

	glutPostRedisplay();
}

void move()
{
	if (fabs(eyex + speed*dir[0]) < GSZ / 2)
		eyex += speed*dir[0];
	if (fabs(eyez + speed*dir[2]) < GSZ / 2)
		eyez += speed*dir[2];
}

void special_keys(int key, int x, int y)
{
	switch (key) {
	case GLUT_KEY_LEFT:
		sight_angle += 0.05;
		break;
	case GLUT_KEY_RIGHT:
		sight_angle -= 0.05;
		break;
	case GLUT_KEY_UP:
		speed = 0.5;
		move();
		break;
	case GLUT_KEY_DOWN:
		speed = -0.5;
		move();
		break;
	case GLUT_KEY_PAGE_UP:
		eyey += 0.5;
		break;
	case GLUT_KEY_PAGE_DOWN:
		if (eyey - 1 > ground[(int)eyez + GSZ / 2][(int)eyex + GSZ / 2] && eyey -1>= 1)
			eyey -= 0.5;
		break;
	}
}

void main(int argc, char* argv[])
{
	glutInit(&argc, argv);
	// There will be the following display buffers:
	// 1. Frame buffer: this is the memory that stores pixel colors for all the screen
	// 2. there are 2 such buffers
	// 3. Depth buffer (Z-buffer)
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
	glutInitWindowSize(W, H);
	glutInitWindowPosition(200, 80);
	glutCreateWindow("Mansion");

	glutDisplayFunc(display);// refresh function
	glutIdleFunc(idle); // timer function
	glutSpecialFunc(special_keys);

	init();

	glutMainLoop();
}
