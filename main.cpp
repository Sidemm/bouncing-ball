//
//  Display a rotating cube 
//

#include "Angel.h"
#include "math.h"

#define N 4  // number of subdivisions
#define M 16*64*3  // number of resulting points


#define CUBE 0
#define SPHERE 1

typedef vec4  color4;
typedef vec4  point4;

const int NumVertices = 36; //(6 faces)(2 triangles/face)(3 vertices/triangle)

point4 points[NumVertices];
color4 colors[NumVertices];

point4 points2[M];



// Array of rotation angles (in degrees) for each coordinate axis
enum { Xaxis = 0, Yaxis = 1, Zaxis = 2, NumAxes = 3 };
int  Axis = Yaxis;
GLfloat  Theta[NumAxes] = { 0.0, 30.0, 40.0 };


// Vertices of a unit cube centered at origin, sides aligned with axes
point4 vertices[8] = {
	point4(-0.1, -0.1,  0.1, 1.0),
	point4(-0.1,  0.1,  0.1, 1.0),
	point4(0.1,  0.1,  0.1, 1.0),
	point4(0.1, -0.1,  0.1, 1.0),
	point4(-0.1, -0.1, -0.1, 1.0),
	point4(-0.1,  0.1, -0.1, 1.0),
	point4(0.1,  0.1, -0.1, 1.0),
	point4(0.1, -0.1, -0.1, 1.0)
};

point4 vertices2[8] = {
	point4(-0.2, -0.2,  0.2, 1.0),
	point4(-0.2,  0.2,  0.2, 1.0),
	point4(0.2,  0.2,  0.2, 1.0),
	point4(0.2, -0.2,  0.2, 1.0),
	point4(-0.2, -0.2, -0.2, 1.0),
	point4(-0.2,  0.2, -0.2, 1.0),
	point4(0.2,  0.2, -0.2, 1.0),
	point4(0.2, -0.2, -0.2, 1.0)
};



enum color { BLACK, RED, YELLOW, GREEN, BLUE, MAGENTA, WHITE, CYAN };

// RGBA olors
color4 vertex_colors[8] = {
	color4(0.0, 0.0, 0.0, 1.0),  // black
	color4(1.0, 0.0, 0.0, 1.0),  // red
	color4(1.0, 1.0, 0.0, 1.0),  // yellow
	color4(0.0, 1.0, 0.0, 1.0),  // green
	color4(0.0, 0.0, 1.0, 1.0),  // blue
	color4(1.0, 0.0, 1.0, 1.0),  // magenta
	color4(1.0, 1.0, 1.0, 1.0),  // white
	color4(0.0, 1.0, 1.0, 1.0)   // cyan
};

// Model-view and projection matrices uniform location
GLuint  ModelView, Projection;


float velocity;
float position;
float gravity;
float timer;
float lastposition;
bool flag; //moving up or moving down
GLint Color;
mat4  model_view1, model_view2;
GLuint vao[2]; //2 pointer for cube and sphere
int Object;


//----------------------------------------------------------------------------

// quad generates two triangles for each face and assigns colors
//    to the vertices

int Index = 0;

void
quad(int a, int b, int c, int d)
{
	// Initialize colors

	points[Index] = vertices[a]; Index++;
	points[Index] = vertices[b]; Index++;
    points[Index] = vertices[c]; Index++;
	points[Index] = vertices[a]; Index++;
	points[Index] = vertices[c]; Index++;
	points[Index] = vertices[d]; Index++;


}

//----------------------------------------------------------------------------

// generate 12 triangles: 36 vertices and 36 colors
void
colorcube()
{
	quad(1, 0, 3, 2);
	quad(2, 3, 7, 6);
	quad(3, 0, 4, 7);
	quad(6, 5, 1, 2);
	quad(4, 5, 6, 7);
	quad(5, 4, 0, 1);
}

//----------------------------CIRCLE-----------------------------------------

point4 v[4] = { vec4(0.0, 0.0, 1.0, 1.0), vec4(0.0, 0.942809, -0.333333, 1.0),
vec4(-0.816497, -0.471405, -0.333333, 1.0),
vec4(0.816497, -0.471405, -0.333333, 1.0) };


static int k = 0;


// move a point to unit circle

point4 unit(const point4 &p)
{
	point4 c;
	double d = 0.0;
	for (int i = 0; i<3; i++) d += p[i] * p[i];
	d = sqrt(d);
	if (d > 0.0) for (int i = 0; i<3; i++) c[i] = p[i] / d;
	c[3] = 1.0;
	return c;
}

void triangle(point4  a, point4 b, point4 c)
{
	points2[k] = a;
	k++;
	points2[k] = b;
	k++;
	points2[k] = c;
	k++;
}


void divide_triangle(point4 a, point4 b, point4 c, int n)
{
	point4 v1, v2, v3;
	if (n>0)
	{
		v1 = unit(a + b);
		v2 = unit(a + c);
		v3 = unit(b + c);
		divide_triangle(a, v2, v1, n - 1);
		divide_triangle(c, v3, v2, n - 1);
		divide_triangle(b, v1, v3, n - 1);
		divide_triangle(v1, v2, v3, n - 1);
	}
	else triangle(a, b, c);
}

void tetrahedron(int n)
{
	divide_triangle(v[0], v[1], v[2], n);
	divide_triangle(v[3], v[2], v[1], n);
	divide_triangle(v[0], v[3], v[1], n);
	divide_triangle(v[0], v[3], v[2], n);
}


// OpenGL initialization
void
init()
{

	velocity = 0.0;
	gravity = -9.8;
	position = 0.0;
	timer = 0.0;
	lastposition = 0.0;
	flag = true;
	Object = 0;
	colorcube();
	tetrahedron(N);
	
	GLuint program = InitShader("vshader.glsl", "fshader.glsl");

	// Create a vertex array object
	glGenVertexArrays(2, &vao[0]); //two vertex arrays created
	glBindVertexArray(vao[0]);

	// Create and initialize a buffer object
	GLuint buffer1; //buffer for the cube
	glGenBuffers(1, &buffer1);
	glBindBuffer(GL_ARRAY_BUFFER, buffer1);
	glBufferData(GL_ARRAY_BUFFER, sizeof(points) + sizeof(colors), NULL, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(points), points);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(points), sizeof(colors), colors);

	// Load shaders and use the resulting shader program

	// set up vertex arrays
	GLuint vPosition1 = glGetAttribLocation(program, "vPosition");
	glEnableVertexAttribArray(vPosition1);
	glVertexAttribPointer(vPosition1, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
	
	glBindVertexArray(vao[1]);
	GLuint buffer2; //buffer for the sphere
	glGenBuffers(1, &buffer2);
	glBindBuffer(GL_ARRAY_BUFFER, buffer2);
	glBufferData(GL_ARRAY_BUFFER, sizeof(points2), points2, GL_STATIC_DRAW);

	GLuint vPosition2 = glGetAttribLocation(program, "vPosition");
	glEnableVertexAttribArray(vPosition2);
	glVertexAttribPointer(vPosition2, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

	GLuint vColor = glGetAttribLocation(program, "vColor");
	glEnableVertexAttribArray(vColor);
	glVertexAttribPointer(vColor, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(sizeof(points)));

	// Retrieve transformation uniform variable locations
	ModelView = glGetUniformLocation(program, "ModelView");
	Projection = glGetUniformLocation(program, "Projection");

	// Set current program object
	glUseProgram(program);

	// Set projection matrix
	mat4  projection;
	projection = Ortho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0); // Ortho(): user-defined function in mat.h
														 //projection = Perspective( 45.0, 1.0, 0.5, 3.0 );
	glUniformMatrix4fv(Projection, 1, GL_TRUE, projection);

	model_view1 = identity();
	glUniformMatrix4fv(ModelView, 1, GL_TRUE, model_view1);
	model_view2 = identity();

	// Enable hiddden surface removal
	glEnable(GL_DEPTH_TEST);

	// Set state variable "clear color" to clear buffer with.
	glClearColor(1.0, 1.0, 1.0, 1.0);

	//
	Color = glGetUniformLocation(program, "Color");


}

//----------------------------------------------------------------------------

void
display(void)
{

	
	// V=V.t(time elapsed between ground and max height)-g*t^2
	velocity =sqrt(lastposition*2/gravity)*gravity-gravity*(glutGet(GLUT_ELAPSED_TIME) - timer)*0.001;

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	if (flag == true) //moving down
		position = 0.5*gravity*(glutGet(GLUT_ELAPSED_TIME) - timer)*(glutGet(GLUT_ELAPSED_TIME) - timer)*0.000001;

	if(flag==false)  //moving up
		position = -((velocity*((glutGet(GLUT_ELAPSED_TIME) - timer)*0.001)) +(0.5*gravity*((glutGet(GLUT_ELAPSED_TIME) - timer)*0.001)*((glutGet(GLUT_ELAPSED_TIME) - timer)*0.001)))+lastposition ;
	
	if (position < -1.6) { //when object is on the ground 
		timer = glutGet(GLUT_ELAPSED_TIME);	
		lastposition = position;
		flag = false;
	}
	if (position == 0.0) { // when object is at max height 
		timer = glutGet(GLUT_ELAPSED_TIME);
		flag = true;
	}
	
	vec3 displacement(0.0, position+0.7, 0.0); //positions arranged with respect to the opengl window
	vec3 displacementsphere(0.0, (position+0.5)*5, 0.0);

	// Scale(), Translate(), RotateX(), RotateY(), RotateZ(): user-defined functions in mat.h

	
	if (Object == CUBE) {
		glBindVertexArray(vao[0]); //display cube
		model_view1 = (Scale(1.0, 1.0, 1.0) * Translate(displacement)*
			RotateX(Theta[Xaxis]) *
			RotateY(Theta[Yaxis]) *
			RotateZ(Theta[Zaxis]));
		glUniformMatrix4fv(ModelView, 1, GL_TRUE, model_view1);//  Generate the model-view matrix of cube
		glDrawArrays(GL_TRIANGLES, 0, NumVertices);

	}
	else if (Object == SPHERE) {
		glBindVertexArray(vao[1]); //display sphere
		model_view2 = (Scale(0.2, 0.2, 0.2) * Translate(displacementsphere)*
			RotateX(Theta[Xaxis]) *
			RotateY(Theta[Yaxis]) *
			RotateZ(Theta[Zaxis])
			);
		glUniformMatrix4fv(ModelView, 1, GL_TRUE, model_view2);//  Generate the model-view matrix of sphere
		glDrawArrays(GL_TRIANGLES, 0, M);
	}
	glutSwapBuffers();

}

//----------------------------------------------------------------------------

void
idle(void)
{
	Theta[Yaxis] += 0.02;
	Theta[Zaxis] = 45.0;


	if (Theta[Xaxis] > 360.0) {
		Theta[Xaxis] -= 360.0;
	}

	glutPostRedisplay();
}

//----------------------------------------------------------------------------

void
keyboard(unsigned char key, int x, int y)
{
	//Up and down arrow inputs are handled in ArrowInput() function

	if (key == 'Q' | key == 'q')
		exit(0);
	if (key == 'i' | key == 'I')
	{
		velocity = 0;
		timer = glutGet(GLUT_ELAPSED_TIME);
		flag = true;

	}
	if (key == 'h' | key == 'H')
	{
		std::cout << "Available input commands:" << std::endl;
		std::cout << "Q: Exit program" << std::endl;
		std::cout << "Up Key(W): Speeds up movement" << std::endl;
		std::cout << "Down Key(S): Slows down movement" << std::endl;
		std::cout << "I: Return initial position" << std::endl;
	}
}

//----------------------------------------------------------------------------
void reshape(int w, int h)
{
	glViewport(0, 0, w, h);

	mat4  projection;
	if (w <= h)
		projection = Ortho(-1.0, 1.0, -1.0 * (GLfloat)h / (GLfloat)w,
			1.0 * (GLfloat)h / (GLfloat)w, -1.0, 1.0);
	else  projection = Ortho(-1.0* (GLfloat)w / (GLfloat)h, 1.0 *
		(GLfloat)w / (GLfloat)h, -1.0, 1.0, -1.0, 1.0);
	glUniformMatrix4fv(Projection, 1, GL_TRUE, projection);

}

//----------------------------------------------------------------------------
void selectColor(int color)
{
	glUniform4f(Color, vertex_colors[color].x, vertex_colors[color].y, vertex_colors[color].z, vertex_colors[color].w);
}
void selectDraw(int draw) {
	if(draw==0)
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	else
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);


}
void selectObject(int selection) {
	Object = selection;
}

//For detecting keyboard inputs up and down arrow 
void ArrowInput(int key, int x, int y)
{
	switch (key)
	{
	case GLUT_KEY_UP:
		gravity = gravity*1.5;

		break;
	case GLUT_KEY_DOWN:
		gravity = gravity / 1.5;

		break;
	}
}
int
main(int argc, char **argv)
{

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
	glutInitWindowSize(512, 512);
	glutCreateWindow("Color Cube");
	glutSpecialFunc(ArrowInput);

	glewExperimental = GL_TRUE;
	glewInit();

	init();

	glutDisplayFunc(display); // set display callback function
	glutReshapeFunc(reshape);
	glutIdleFunc(idle);
	glutKeyboardFunc(keyboard);

	int colorMenu = glutCreateMenu(selectColor); 
	glutAddMenuEntry("black", BLACK);
	glutAddMenuEntry("red", RED);
	glutAddMenuEntry("yellow", YELLOW);
	glutAddMenuEntry("green", GREEN);
	glutAddMenuEntry("blue", BLUE);
	glutAddMenuEntry("magenta", MAGENTA);
	glutAddMenuEntry("cyan", CYAN);
	
	int drawMenu = glutCreateMenu(selectDraw);
	glutAddMenuEntry("solid", 1);
	glutAddMenuEntry("wireframe", 0);

	int objectMenu = glutCreateMenu(selectObject);
	glutAddMenuEntry("cube", 0);
	glutAddMenuEntry("sphere", 1);

	glutCreateMenu(0);
	glutAddSubMenu("colors", colorMenu);
	glutAddSubMenu("draw", drawMenu);
	glutAddSubMenu("object", objectMenu);

	glutAttachMenu(GLUT_LEFT_BUTTON);

	glutMainLoop();

	return 0;
}
