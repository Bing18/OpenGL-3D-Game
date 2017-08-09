#include <iostream>
#include <cmath>
#include <fstream>
#include <vector>
#include <string.h>
#include <sstream>
#include <thread>
#include <ao/ao.h>
#include <mpg123.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace std;

struct VAO {
    GLuint VertexArrayID;
    GLuint VertexBuffer;
    GLuint ColorBuffer;

    GLenum PrimitiveMode;
    GLenum FillMode;
    int NumVertices;
};
typedef struct VAO VAO;

struct GLMatrices {
	glm::mat4 projection;
  glm::mat4 projection1;
	glm::mat4 model;
	glm::mat4 view;
	GLuint MatrixID;
} Matrices;

GLuint programID;

struct Tiles{
  int type;
  int status;
  VAO *tile;
  int yeffect;
};
struct Tiles board[10][15];

struct Blockstruct{
  int x;
  int y;
  VAO *block;
  int movx;
  int movy;
  int movz;
};
struct Blockstruct movblock[10];
struct Segmentstruct{
  float x;
  float y;
  float z;
  int status;
  VAO *object;
};
struct Segmentstruct segment[30];
double utime=glfwGetTime();
double utime1=glfwGetTime();
int totalblocks=0;
int currscore=0;
int mouseflag=0;
int width=1400;
int height=800;
/* Function to load Shaders - Use it as it is */
GLuint LoadShaders(const char * vertex_file_path,const char * fragment_file_path) {
	// Create the shaders
	GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

	// Read the Vertex Shader code from the file
	std::string VertexShaderCode;
	std::ifstream VertexShaderStream(vertex_file_path, std::ios::in);
	if(VertexShaderStream.is_open())
	{
		std::string Line = "";
		while(getline(VertexShaderStream, Line))
			VertexShaderCode += "\n" + Line;
		VertexShaderStream.close();
	}

	// Read the Fragment Shader code from the file
	std::string FragmentShaderCode;
	std::ifstream FragmentShaderStream(fragment_file_path, std::ios::in);
	if(FragmentShaderStream.is_open()){
		std::string Line = "";
		while(getline(FragmentShaderStream, Line))
			FragmentShaderCode += "\n" + Line;
		FragmentShaderStream.close();
	}

	GLint Result = GL_FALSE;
	int InfoLogLength;

	// Compile Vertex Shader
	printf("Compiling shader : %s\n", vertex_file_path);
	char const * VertexSourcePointer = VertexShaderCode.c_str();
	glShaderSource(VertexShaderID, 1, &VertexSourcePointer , NULL);
	glCompileShader(VertexShaderID);

	// Check Vertex Shader
	glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> VertexShaderErrorMessage(InfoLogLength);
	glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
	fprintf(stdout, "%s\n", &VertexShaderErrorMessage[0]);

	// Compile Fragment Shader
	printf("Compiling shader : %s\n", fragment_file_path);
	char const * FragmentSourcePointer = FragmentShaderCode.c_str();
	glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer , NULL);
	glCompileShader(FragmentShaderID);

	// Check Fragment Shader
	glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> FragmentShaderErrorMessage(InfoLogLength);
	glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
	fprintf(stdout, "%s\n", &FragmentShaderErrorMessage[0]);

	// Link the program
	fprintf(stdout, "Linking program\n");
	GLuint ProgramID = glCreateProgram();
	glAttachShader(ProgramID, VertexShaderID);
	glAttachShader(ProgramID, FragmentShaderID);
	glLinkProgram(ProgramID);

	// Check the program
	glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
	glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> ProgramErrorMessage( max(InfoLogLength, int(1)) );
	glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
	fprintf(stdout, "%s\n", &ProgramErrorMessage[0]);

	glDeleteShader(VertexShaderID);
	glDeleteShader(FragmentShaderID);

	return ProgramID;
}

static void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error: %s\n", description);
}

void quit(GLFWwindow *window)
{
    glfwDestroyWindow(window);
    glfwTerminate();
//    exit(EXIT_SUCCESS);
}
void* play_audio(string audioFile);

void* play_audio(string audioFile){
	mpg123_handle *mh;
	unsigned char *buffer;
	size_t buffer_size;
	size_t done;
	int err;

	int driver;
	ao_device *dev;

	ao_sample_format format;
	int channels, encoding;
	long rate;

	/* initializations */
	ao_initialize();
	driver = ao_default_driver_id();
	mpg123_init();
	mh = mpg123_new(NULL, &err);
	buffer_size = mpg123_outblock(mh);
	buffer = (unsigned char*) malloc(buffer_size * sizeof(unsigned char));

	/* open the file and get the decoding format */
	mpg123_open(mh, &audioFile[0]);
	mpg123_getformat(mh, &rate, &channels, &encoding);

	/* set the output format and open the output device */
	format.bits = mpg123_encsize(encoding) * 8;
	format.rate = rate;
	format.channels = channels;
	format.byte_format = AO_FMT_NATIVE;
	format.matrix = 0;
	dev = ao_open_live(driver, &format, NULL);

	/* decode and play */
	char *p =(char *)buffer;
	while (mpg123_read(mh, buffer, buffer_size, &done) == MPG123_OK)
		ao_play(dev, p, done);

	/* clean up */
	free(buffer);
	ao_close(dev);
	mpg123_close(mh);
	mpg123_delete(mh);
}


/* Generate VAO, VBOs and return VAO handle */
struct VAO* create3DObject (GLenum primitive_mode, int numVertices, const GLfloat* vertex_buffer_data, const GLfloat* color_buffer_data, GLenum fill_mode=GL_FILL)
{
    struct VAO* vao = new struct VAO;
    vao->PrimitiveMode = primitive_mode;
    vao->NumVertices = numVertices;
    vao->FillMode = fill_mode;

    // Create Vertex Array Object
    // Should be done after CreateWindow and before any other GL calls
    glGenVertexArrays(1, &(vao->VertexArrayID)); // VAO
    glGenBuffers (1, &(vao->VertexBuffer)); // VBO - vertices
    glGenBuffers (1, &(vao->ColorBuffer));  // VBO - colors

    glBindVertexArray (vao->VertexArrayID); // Bind the VAO
    glBindBuffer (GL_ARRAY_BUFFER, vao->VertexBuffer); // Bind the VBO vertices
    glBufferData (GL_ARRAY_BUFFER, 3*numVertices*sizeof(GLfloat), vertex_buffer_data, GL_STATIC_DRAW); // Copy the vertices into VBO
    glVertexAttribPointer(
                          0,                  // attribute 0. Vertices
                          3,                  // size (x,y,z)
                          GL_FLOAT,           // type
                          GL_FALSE,           // normalized?
                          0,                  // stride
                          (void*)0            // array buffer offset
                          );

    glBindBuffer (GL_ARRAY_BUFFER, vao->ColorBuffer); // Bind the VBO colors
    glBufferData (GL_ARRAY_BUFFER, 3*numVertices*sizeof(GLfloat), color_buffer_data, GL_STATIC_DRAW);  // Copy the vertex colors
    glVertexAttribPointer(
                          1,                  // attribute 1. Color
                          3,                  // size (r,g,b)
                          GL_FLOAT,           // type
                          GL_FALSE,           // normalized?
                          0,                  // stride
                          (void*)0            // array buffer offset
                          );

    return vao;
}


/* Generate VAO, VBOs and return VAO handle - Common Color for all vertices */
struct VAO* create3DObject (GLenum primitive_mode, int numVertices, const GLfloat* vertex_buffer_data, const GLfloat red, const GLfloat green, const GLfloat blue, GLenum fill_mode=GL_FILL)
{
    GLfloat* color_buffer_data = new GLfloat [3*numVertices];
    for (int i=0; i<numVertices; i++) {
        color_buffer_data [3*i] = red;
        color_buffer_data [3*i + 1] = green;
        color_buffer_data [3*i + 2] = blue;
    }

    return create3DObject(primitive_mode, numVertices, vertex_buffer_data, color_buffer_data, fill_mode);
}

/* Render the VBOs handled by VAO */
void draw3DObject (struct VAO* vao)
{
    // Change the Fill Mode for this object
    glPolygonMode (GL_FRONT_AND_BACK, vao->FillMode);

    // Bind the VAO to use
    glBindVertexArray (vao->VertexArrayID);

    // Enable Vertex Attribute 0 - 3d Vertices
    glEnableVertexAttribArray(0);
    // Bind the VBO to use
    glBindBuffer(GL_ARRAY_BUFFER, vao->VertexBuffer);

    // Enable Vertex Attribute 1 - Color
    glEnableVertexAttribArray(1);
    // Bind the VBO to use
    glBindBuffer(GL_ARRAY_BUFFER, vao->ColorBuffer);

    // Draw the geometry !
    glDrawArrays(vao->PrimitiveMode, 0, vao->NumVertices); // Starting from vertex 0; 3 vertices total -> 1 triangle
}

/**************************
 * Customizable functions *
 **************************/

float triangle_rot_dir = 1;
float rectangle_rot_dir = 1;
bool triangle_rot_status = true;
bool rectangle_rot_status = true;
void level1();
void set();
int level=1;
/* Executed when a regular key is pressed/released/held-down */
/* Prefered for Keyboard events */
int l8f=0;
int disable=0;
float blockeffect;
int points=0;
float hx,hy,hz;
void keyboard (GLFWwindow* window, int key, int scancode, int action, int mods)
{
     // Function is called first on GLFW_PRESS.

    if (action == GLFW_RELEASE) {
        switch (key) {
            case GLFW_KEY_C:
                rectangle_rot_status = !rectangle_rot_status;
                break;
            case GLFW_KEY_P:
                triangle_rot_status = !triangle_rot_status;
                break;
            case GLFW_KEY_X:
                // do something ..
                break;
            default:
                break;
        }
    }
    else if (action == GLFW_PRESS) {
        if(key==GLFW_KEY_T){
          mouseflag=1;
        }
        if(key==GLFW_KEY_R){
          mouseflag=0;
        }
        if(key==GLFW_KEY_H){
          mouseflag=2;
          hx=0;
          hy=60;
          hz=70;
        }
        if(key==GLFW_KEY_RIGHT && !disable){
          currscore++;
          if(l8f==0){
            if(movblock[0].movx==movblock[1].movx){
              if(movblock[0].movy>movblock[1].movy){
                movblock[0].movx+=12;
                movblock[1].movx+=6;
                movblock[0].movy-=6;
              }
              else if(movblock[0].movy<movblock[1].movy){
                movblock[0].movx+=6;
                movblock[1].movx+=12;
                movblock[1].movy-=6;
              }
              else{
                movblock[0].movx+=6;
                movblock[1].movx+=6;
              }
          }
          else if(movblock[0].movx>movblock[1].movx&& movblock[0].movy==movblock[1].movy){
      			movblock[0].movx+=6;
      			movblock[1].movx+=12;
      			movblock[1].movy+=6;
      		}
      		else if(movblock[0].movx<movblock[1].movx && movblock[0].movy==movblock[1].movy){
            movblock[0].movx+=12;
      			movblock[1].movx+=6;
      			movblock[0].movy+=6;
          }
        }
        else if(l8f==1)
          movblock[1].movx+=6;
        else if(l8f==2){
        //  printf("aa\n");
          movblock[0].movx+=6;
        }
        }
        if(key==GLFW_KEY_LEFT && !disable){
          currscore++;
          if(l8f==0){
            if(movblock[0].movx==movblock[1].movx){
              if(movblock[0].movy>movblock[1].movy){
                movblock[0].movx-=12;
                movblock[1].movx-=6;
                movblock[0].movy-=6;
              }
              else if(movblock[0].movy<movblock[1].movy){
                movblock[0].movx-=6;
                movblock[1].movx-=12;
                movblock[1].movy-=6;
              }
              else{
                movblock[0].movx-=6;
                movblock[1].movx-=6;
              }
          }
          else if(movblock[0].movx>movblock[1].movx&& movblock[0].movy==movblock[1].movy){
      			movblock[0].movx-=12;
      			movblock[1].movx-=6;
      			movblock[0].movy+=6;
      		}
      		else if(movblock[0].movx<movblock[1].movx && movblock[0].movy==movblock[1].movy){
            movblock[0].movx-=6;
      			movblock[1].movx-=12;
      			movblock[1].movy+=6;
          }
        }
        else if(l8f==1)
          movblock[1].movx-=6;
        else if(l8f==2)
          movblock[0].movx-=6;
        }
        else if(key==GLFW_KEY_UP && !disable){
          currscore++;
      		if(l8f==0)
      		{
            if(movblock[0].movz==movblock[1].movz){
              if(movblock[0].movy>movblock[1].movy){
                movblock[0].movz-=12;
                movblock[1].movz-=6;
                movblock[0].movy-=6;
              }
              else if(movblock[0].movy<movblock[1].movy){
                movblock[0].movz-=6;
                movblock[1].movz-=12;
                movblock[1].movy-=6;
              }
              else{
                movblock[0].movz-=6;
                movblock[1].movz-=6;
              }
          }
          else if(movblock[0].movz>movblock[1].movz&& movblock[0].movy==movblock[1].movy){
            movblock[0].movz-=12;
            movblock[1].movz-=6;
            movblock[0].movy+=6;
          }
          else if(movblock[0].movz<movblock[1].movz && movblock[0].movy==movblock[1].movy){
            movblock[0].movz-=6;
            movblock[1].movz-=12;
            movblock[1].movy+=6;
          }
        }
        else if(l8f==1)
          movblock[1].movz-=6;
        else if(l8f==2)
          movblock[0].movz-=6;
      		}
      	if(key==GLFW_KEY_DOWN && !disable){
          currscore++;
          if(l8f==0)
      		{
            if(movblock[0].movz==movblock[1].movz){
              if(movblock[0].movy>movblock[1].movy){
                movblock[0].movz+=12;
                movblock[1].movz+=6;
                movblock[0].movy-=6;
              }
              else if(movblock[0].movy<movblock[1].movy){
                movblock[0].movz+=6;
                movblock[1].movz+=12;
                movblock[1].movy-=6;
              }
              else{
                movblock[0].movz+=6;
                movblock[1].movz+=6;
              }
          }
          else if(movblock[0].movz>movblock[1].movz&& movblock[0].movy==movblock[1].movy){
            movblock[0].movz+=6;
            movblock[1].movz+=12;
            movblock[1].movy+=6;
          }
          else if(movblock[0].movz<movblock[1].movz && movblock[0].movy==movblock[1].movy){
            movblock[1].movz+=6;
            movblock[0].movz+=12;
            movblock[0].movy+=6;
          }
        }
        else if(l8f==1)
          movblock[1].movz+=6;
        else if(l8f==2)
          movblock[0].movz+=6;
      	}
        switch (key) {
            case GLFW_KEY_ESCAPE:
                quit(window);
                break;
            default:
                break;
        }
    }
}

/* Executed for character input (like in text boxes) */
void keyboardChar (GLFWwindow* window, unsigned int key)
{
	switch (key) {
		case 'Q':
		case 'q':
            quit(window);
            break;
		default:
			break;
	}
}
double mouse_xpos,mouse_ypos,mouse_click_x;
static void cursor_position(GLFWwindow* window,double xpos,double ypos)
{
	mouse_xpos=(20*(xpos+35)/width)-10;
	mouse_ypos=-(20*(ypos+85)/height)+10;
  ///printf("%d %lf %lf\n",6*(movblock[0].y-4),mouse_xpos,mouse_ypos);
}
int mouseclick=0;
/* Executed when a mouse button is pressed/released */
void mouseButton (GLFWwindow* window, int button, int action, int mods)
{
    switch (button) {
        case GLFW_MOUSE_BUTTON_LEFT:
            if (action == GLFW_RELEASE)
                triangle_rot_dir *= -1;
            break;
        case GLFW_MOUSE_BUTTON_RIGHT:
            if (action == GLFW_PRESS && mouseflag==2) {
                if(mouse_xpos<10 && mouse_xpos>-9.5){
                //  hx=mouse_xpos;
                  //hy=mouse_ypos;
              //    printf("yes\n");
                  mouseclick=1;
                }

            }
            if(action==GLFW_RELEASE){
              mouseclick=0;
            }
            break;
        default:
            break;
    }
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{

}

/* Executed when window is resized to 'width' and 'height' */
/* Modify the bounds of the screen here in glm::ortho or Field of View in glm::Perspective */
void reshapeWindow (GLFWwindow* window, int width, int height)
{
    int fbwidth=width, fbheight=height;
    /* With Retina display on Mac OS X, GLFW's FramebufferSize
     is different from WindowSize */
    glfwGetFramebufferSize(window, &fbwidth, &fbheight);

	GLfloat fov = 0.9f;

	// sets the viewport of openGL renderer
	glViewport (0, 0, (GLsizei) fbwidth, (GLsizei) fbheight);

	// set the projection matrix as perspective
	/* glMatrixMode (GL_PROJECTION);
	   glLoadIdentity ();
	   gluPerspective (fov, (GLfloat) fbwidth / (GLfloat) fbheight, 0.1, 500.0); */
	// Store the projection matrix in a variable for future use
    // Perspective projection for 3D views
     Matrices.projection = glm::perspective (fov, (GLfloat) fbwidth / (GLfloat) fbheight, 0.1f, 500.0f);

    // Ortho projection for 2D views
    Matrices.projection1 = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, 0.1f, 500.0f);
}
void set_segments(int letter_cnt,char letter)
{
	for(int i=0;i<9;i++)
		segment[9*letter_cnt+i].status=0;

	//left1
	if(letter=='O' || letter=='M' || letter=='G' || letter=='A' || letter=='U'|| letter=='E' || letter=='0'|| letter=='4'|| letter=='5'|| letter=='6'|| letter=='8'|| letter=='9' || letter=='S' || letter=='P' || letter=='N' || letter=='Y' || letter=='W')
	{
		segment[9*letter_cnt+0].status=1;
	}
	//middle1
	if(letter=='1'|| letter=='M' || letter=='T' || letter=='I')
	{
		segment[9*letter_cnt+1].status=1;
	}
	//right1
	if(letter=='U' || letter=='O' || letter=='M'|| letter=='A'|| letter=='0'|| letter=='2'|| letter=='3'|| letter=='4'|| letter=='8'|| letter=='7'|| letter=='9' ||letter=='P' || letter=='N'|| letter=='Y' || letter=='W')
	{
		segment[9*letter_cnt+2].status=1;
	}
	//left2
	if(letter=='O' || letter=='M'  || letter=='A'|| letter=='G'|| letter=='U' ||letter=='E'|| letter=='0'|| letter=='2'||  letter=='6'|| letter=='8' || letter=='P' || letter=='N'|| letter=='W')
	{
		segment[9*letter_cnt+3].status=1;
	}
	//middle2
	if(letter=='1' || letter=='T' || letter=='I' || letter=='W')
	{
		segment[9*letter_cnt+4].status=1;
	}
	//right2
	if(letter=='O' ||  letter=='A' || letter=='G' ||letter=='M'|| letter=='U'|| letter=='0'|| letter=='3'|| letter=='5'|| letter=='6'|| letter=='4'|| letter=='8'|| letter=='7'|| letter=='9' || letter=='S' || letter=='N' || letter=='Y' || letter=='W')
	{
		segment[9*letter_cnt+5].status=1;
	}
	//top
	if(letter=='O' || letter=='M' || letter=='A' || letter=='G'|| letter=='E'|| letter=='0'|| letter=='2'|| letter=='3'|| letter=='5'|| letter=='6'|| letter=='8' || letter=='7'|| letter=='9' || letter=='T' || letter=='S' || letter=='P' || letter=='I' || letter=='N')
	{
		segment[9*letter_cnt+6].status=1;
	}
	//middle
	if(letter=='G'  || letter=='A' || letter=='E'|| letter=='-' || letter=='2'|| letter=='3'|| letter=='4'|| letter=='5'|| letter=='6'|| letter=='8'|| letter=='9' || letter=='S' || letter=='P' || letter=='Y')
	{
		segment[9*letter_cnt+7].status=1;
	}
	//bottom
	if(letter=='O' || letter=='G' || letter=='U' || letter=='E'|| letter=='0'|| letter=='2'|| letter=='3'|| letter=='5'|| letter=='6'|| letter=='8'|| letter=='9' || letter=='S'|| letter=='I' || letter=='W')
	{
		segment[9*letter_cnt+8].status=1;
	}
}


VAO *triangle, *rectangle,*cube,*border;

// Creates the triangle object used in this sample code
void createTriangle ()
{
  /* ONLY vertices between the bounds specified in glm::ortho will be visible on screen */

  /* Define vertex array as used in glBegin (GL_TRIANGLES) */
  static const GLfloat vertex_buffer_data [] = {
    0, 1,0, // vertex 0
    -1,-1,0, // vertex 1
    1,-1,0, // vertex 2
  };

  static const GLfloat color_buffer_data [] = {
    1,0,0, // color 0
    0,1,0, // color 1
    0,0,1, // color 2
  };

  // create3DObject creates and returns a handle to a VAO that can be used later
  triangle = create3DObject(GL_TRIANGLES, 3, vertex_buffer_data, color_buffer_data, GL_LINE);
}

// Creates the rectangle object used in this sample code
void createRectangle ()
{
  // GL3 accepts only Triangles. Quads are not supported
  static const GLfloat vertex_buffer_data [] = {
    -1.2,-1,0, // vertex 1
    1.2,-1,0, // vertex 2
    1.2, 1,0, // vertex 3

    1.2, 1,0, // vertex 3
    -1.2, 1,0, // vertex 4
    -1.2,-1,0  // vertex 1
  };

  static const GLfloat color_buffer_data [] = {
    1,0,0, // color 1
    0,0,1, // color 2
    0,1,0, // color 3

    0,1,0, // color 3
    0.3,0.3,0.3, // color 4
    1,0,0  // color 1
  };

  // create3DObject creates and returns a handle to a VAO that can be used later
  rectangle = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
}
int segment_cnt=0;
void createSegment (GLfloat h,GLfloat w,float x,float y,float z)
{
	GLfloat vertex_buffer_data [] = {
		-w/2,h/2,0, // vertex 1
		w/2,h/2,0, // vertex 2
		w/2,-h/2,0, // vertex 3

		w/2,-h/2,0, // vertex 3
		-w/2,-h/2,0, // vertex 4
		-w/2,h/2,0  // vertex 1
	};
	GLfloat color_buffer_data [] = {
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	};
	segment[segment_cnt].object = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
	segment[segment_cnt].x = x;
	segment[segment_cnt].y = y;
	segment[segment_cnt].z = z;
	segment[segment_cnt].status=0;
	segment_cnt++;
}

void createCube (int type)
{
  // GL3 accepts only Triangles. Quads are not supported
  static const GLfloat vertex_buffer_data [] = {
    -2.0f,-2.0f,-2.0f,
		-2.0f,-2.0f, 2.0f,
		-2.0f, 2.0f, 2.0f,
		2.0f, 2.0f,-2.0f,
		-2.0f,-2.0f,-2.0f,
		-2.0f, 2.0f,-2.0f,
		2.0f,-2.0f, 2.0f,
		-2.0f,-2.0f,-2.0f,
		2.0f,-2.0f,-2.0f,
		2.0f, 2.0f,-2.0f,
		2.0f,-2.0f,-2.0f,
		-2.0f,-2.0f,-2.0f,
		-2.0f,-2.0f,-2.0f,
		-2.0f, 2.0f, 2.0f,
		-2.0f, 2.0f,-2.0f,
		2.0f,-2.0f, 2.0f,
		-2.0f,-2.0f, 2.0f,
		-2.0f,-2.0f,-2.0f,
		-2.0f, 2.0f, 2.0f,
		-2.0f,-2.0f, 2.0f,
		2.0f,-2.0f, 2.0f,
		2.0f, 2.0f, 2.0f,
		2.0f,-2.0f,-2.0f,
		2.0f, 2.0f,-2.0f,
		2.0f,-2.0f,-2.0f,
		2.0f, 2.0f, 2.0f,
		2.0f,-2.0f, 2.0f,
		2.0f, 2.0f, 2.0f,
		2.0f, 2.0f,-2.0f,
		-2.0f, 2.0f,-2.0f,
		2.0f, 2.0f, 2.0f,
		-2.0f, 2.0f,-2.0f,
		-2.0f, 2.0f, 2.0f,
		2.0f, 2.0f, 2.0f,
		-2.0f, 2.0f, 2.0f,
		2.0f,-2.0f, 2.0f
  };
//type: 1-floorcolor1,2-floorcolor2,3-block,4-switch
GLfloat color_buffer_data [12*9];
if(type<3){
if(type==1)
{
  for(int z=0;z<12*3;z++)
  {
    color_buffer_data[3*z]=0.8;
    color_buffer_data[3*z+1]=0.8;
    color_buffer_data[3*z+2]=0.6;
  }
  for(int p=0;p<10;p++)
  {
    for(int q=0;q<15;q++){
      if((p+q)%2==0){
        board[p][q].tile=create3DObject(GL_TRIANGLES,36,vertex_buffer_data,color_buffer_data,GL_FILL);
        board[p][q].type=1;
      }
    }
  }
}
if(type==2)
{
  for(int z=0;z<12*3;z++)
  {
    color_buffer_data[3*z]=1;
    color_buffer_data[3*z+1]=1;
    color_buffer_data[3*z+2]=1;
  }
  for(int p=0;p<10;p++)
  {
    for(int q=0;q<15;q++){
      if((p+q)%2!=0){
        board[p][q].tile=create3DObject(GL_TRIANGLES,36,vertex_buffer_data,color_buffer_data,GL_FILL);
        board[p][q].type=1;
      }
    }
  }
}
}
else{
if(type==3)
{
  for(int z=0;z<12*3;z++)
  {
    color_buffer_data[3*z]=135.0/255.0;//0.6
    color_buffer_data[3*z+1]=54.0/255.0;//0.36
    color_buffer_data[3*z+2]=0.0/255.0;//0.26
  }
}
if(type==4)
{
  for(int z=0;z<12*3;z++)
  {
    color_buffer_data[3*z]=40.0/255.0;
    color_buffer_data[3*z+1]=180.0/255.0;
    color_buffer_data[3*z+2]=99.0/255.0;
  }
}
if(type==5)
{
  for(int z=0;z<12*3;z++)
  {
    color_buffer_data[3*z]=230.0/255.0;
    color_buffer_data[3*z+1]=126.0/255.0;
    color_buffer_data[3*z+2]=34.0/255.0;
  }
}
if(type==6)
{
  for(int z=0;z<12*3;z++)
  {
    color_buffer_data[3*z]=1.0/255.0;
    color_buffer_data[3*z+1]=0.0/255.0;
    color_buffer_data[3*z+2]=0.0/255.0;
  }
}
movblock[totalblocks].block=create3DObject(GL_TRIANGLES,36,vertex_buffer_data,color_buffer_data,GL_FILL);
totalblocks++;
}
  // create3DObject creates and returns a handle to a VAO that can be used later
}
//types : 0:empty,1:present,2:green,3:orange(both as switches),4:goal,5:fragile
void level1(){
  for(int i=0;i<10;i++){
    for(int j=0;j<15;j++){
      if(i==0||i==1||i==8||i==9)
      board[i][j].type=0;
      else{
        break;
      }
    }
  }
  for(int i=2;i<10;i++){
    for(int j=10;j<15;j++){
      board[i][j].type=0;
    }
  }
  for(int j=0;j<8;j++)
  {
    if(j==7)
      board[6][7].type=4;
    else if(j!=5&&j!=6){
      board[6][j].type=0;
      board[7][j].type=0;
    }
  }
  board[7][5].type=0;
  board[7][9].type=0;
  int i=2;
  for(int j=3;j<10;j++)board[i][j].type=0;
  i=3;
  for(int j=6;j<10;j++)
    board[i][j].type=0;
  board[4][9].type=0;
}
int l2bridge1=0,l2bridge2=0,l2tog1=0,l2tog2=0;
void level2(){
  for(int i=0;i<10;i++)
    for(int j=0;j<15;j++)
      board[i][j].type=1;

  for(int i=0;i<10;i++){
    for(int j=0;j<15;j++){
      if(i==0||i==1||i==2||i==8||i==9)
      board[i][j].type=0;
      else{
        break;
      }
    }
  }
  for(int i=3;i<8;i++){
    for(int j=0;j<15;j++){
    if(j==10||j==11||j==4||j==5){
    board[i][j].type=0;
    board[i][j].type=0;
  }
  }
  }
  int i=2;
  for(int j=6;j<15;j++)
    board[i][j].type=1;
  board[2][10].type=0;
  board[2][11].type=0;
  board[7][12].type=0;
  board[7][13].type=0;
  board[7][14].type=0;
  board[3][13].type=4;
  board[3][8].type=3;
  board[4][2].type=2;
  l2bridge1=0;
  l2tog1=0;
  l2tog2=0;
  l2bridge2=0;
}
int l3=0,l6=0,l7=0,r3=0,r4=0,r6=0,r7=0,r8=0,r9=0;
void level3(){
  for(int i=0;i<10;i++)
    for(int j=0;j<15;j++){
      if(i>3 && i<8)
      board[i][j].type=1;
      else
      board[i][j].type=0;
    }
    for(int i=6;i<8;i++){
      for(int j=4;j<11;j++)
      board[i][j].type=0;
    }
    int i=3;
    for(int j=6;j<15;j++)
      board[i][j].type=1;
    i=4;
    for(int j=0;j<15;j++){
      if(j==4||j==5||j==9||j==10||j==13||j==14)
        board[i][j].type=0;
    }
    board[3][13].type=0;
    board[3][14].type=0;
    board[5][9].type=0;
    board[5][10].type=0;
    board[3][13].type=0;
    board[3][14].type=0;
    board[6][13].type=4;
    board[7][11].type=0;
}
int l4bridge=0,l4tog=0;
int fragile_y[2]={0};
int flag=0;
void level4()
{
  flag=0;
  for(int i=0;i<10;i++)
    for(int j=0;j<15;j++){
      if(i>0)
      board[i][j].type=1;
      else
      board[i][j].type=0;
    }
  for(int i=1;i<3;i++){
    for(int j=0;j<15;j++){
      if(j<3 || j>=13)
        board[i][j].type=0;
    }
  }
  for(int i=3;i<6;i++){
    for(int j=4;j<9;j++)
    board[i][j].type=0;
  }
  for(int i=6;i<=7;i++){
    for(int j=8;j<=9;j++)
    board[i][j].type=0;//change to 0
  }
  board[4][3].type=0;
  board[4][9].type=0;
  board[5][3].type=0;
  board[5][9].type=0;
  for(int i=6;i<=7;i++){
    for(int j=3;j<=4;j++)
    board[i][j].type=0;
  }
  for(int i=8;i<10;i++){
    for(int j=0;j<10;j++){
      if(j<5 || j>8)
        board[i][j].type=0;
    }
  }
  board[8][13].type=3;//change to 3
  board[8][6].type=4;
  board[3][9].type=0;
  board[6][10].type=5;//change to 5 (fragile)
  board[3][10].type=5;//change to 5
  board[6][1].type=1;
  board[8][9].type=0;
  board[8][8].type=0;
  board[9][9].type=0;
  board[9][8].type=0;
  l4bridge=0;
  l4tog=0;
  fragile_y[0]=0;
  fragile_y[1]=0;
}
int r5=0;
void level5(){
  for(int i=0;i<10;i++){
    for(int j=0;j<15;j++){
      board[i][j].type=0;
    }
  }
  for(int i=4;i<7;i++){
    for(int j=0;j<15;j++){
      if(j<6 || j>=12)
      board[i][j].type=1;
    }
  }
  for(int i=1;i<10;i++)
  {
    for(int j=9;j<12;j++){
      board[i][j].type=1;
    }
  }
  board[5][13].type=4;
  board[5][4].type=7;
  l8f=0;
}
int blockflag;
void set(){
  currscore=0;
  for(int i=0;i<10;i++){
    for(int j=0;j<15;j++){
      board[i][j].yeffect=-60;
    }
  }
  blockeffect=60;
  for(int i=0;i<2;i++){
    movblock[i].movx=0;
    movblock[i].movy=0;
    movblock[i].movz=0;
  }
  movblock[1].movy=6;
  disable=0;
  blockflag=0;
}
float camera_rotation_angle = 90;
float rectangle_rotation = 0;
float triangle_rotation = 0;

/* Render the scene with openGL */
/* Edit this function according to your assignment */
void draw ()
{
  // clear the color and depth in the frame buffer
  glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // use the loaded shader program
  // Don't change unless you know what you are doing
  glUseProgram (programID);

  // Eye - Location of camera. Don't change unless you are sure!!
  glm::vec3 eye ( 5*cos(camera_rotation_angle*M_PI/180.0f),0, 5*sin(camera_rotation_angle*M_PI/180.0f) );
  // Target - Where is the camera looking at.  Don't change unless you are sure!!
  glm::vec3 target (0, 0, 0);
  // Up - Up vector defines tilt of camera.  Don't change unless you are sure!!
  glm::vec3 up (0, 1, 0);

  // Compute Camera matrix (view)
  // Matrices.view = glm::lookAt( eye, target, up ); // Rotating Camera for 3D
  //  Don't change unless you are sure!!
  if(mouseclick==1){
    hx=mouse_xpos;
  }
  if(mouseflag==1)
  Matrices.view = glm::lookAt(glm::vec3(0,100,1), glm::vec3(0,0,0), glm::vec3(0,1,0)); // Fixed camera for 2D (ortho) in XY plane
  else if(mouseflag==0)
  Matrices.view = glm::lookAt(glm::vec3(-30,70,60), glm::vec3(0,0,0), glm::vec3(0,1,0)); // Fixed camera for 2D (ortho) in XY plane
  else if(mouseflag==2)
  Matrices.view = glm::lookAt(glm::vec3(hx,hy,hz), glm::vec3(0,0,0), glm::vec3(0,1,0)); // Fixed camera for 2D (ortho) in XY plane
  // Compute ViewProject matrix as view/camera might not be changed for this frame (basic scenario)
  //  Don't change unless you are sure!!
  glm::mat4 VP = Matrices.projection * Matrices.view;

  // Send our transformation to the currently bound shader, in the "MVP" uniform
  // For each model you render, since the MVP will be different (at least the M part)
  //  Don't change unless you are sure!!
  glm::mat4 MVP;	// MVP = Projection * View * Model

  for(int i=0;i<10;i++){
    for(int j=0;j<15;j++)
    {
      if((board[i][j].type==1||board[i][j].type==2||board[i][j].type==3||board[i][j].type==5) && level!=6){
        board[i][j].yeffect+=(i+j)/1.5;
        if(board[i][j].yeffect>0)
          board[i][j].yeffect=0;
          Matrices.model = glm::mat4(1.0f);
          glm::mat4 scaletile = glm::scale(glm::vec3(1.5f,0.4f,1.5f));
          glm::mat4 translatetile = glm::translate (glm::vec3((j+1)*6-30,board[i][j].yeffect,(i+1)*6-30));        // glTranslatef
          glm::mat4 rotatetile = glm::rotate((float)(0*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
          Matrices.model *= (translatetile * rotatetile*scaletile);
          MVP = VP * Matrices.model;
          glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
          draw3DObject(board[i][j].tile);

      }
    }
  }
  if(blockeffect>0)
blockeffect-=2;
if(blockeffect==0 && blockflag==0){
  blockflag=1;
  disable=0;
}
if(movblock[0].y<0 || movblock[1].y<0 || blockeffect>0){
  disable=1;
}
for(int i=0;i<2;i++)
{
  movblock[i].x=((-18+movblock[i].movx+l3)/6)+4;
  movblock[i].y=((-6+movblock[i].movz+r3+r5)/6)+4;
}
if(level!=6){
for(int i=0;i<2;i++){
Matrices.model = glm::mat4(1.0f);
glm::mat4 translatecube = glm::translate (glm::vec3(-18.0f+movblock[i].movx+l3, 3.0f+movblock[i].movy+blockeffect, -6.0f+movblock[i].movz+r3+r5)); // glTranslatef

glm::mat4 rotatecube = glm::rotate((float)(0*M_PI/180.0f), glm::vec3(0,0,-3));

glm::mat4 scalecube = glm::scale (glm::vec3(1.5f, 1.5f, 1.5f)); // glTranslatef
// rotate about vector (1,0,0)
glm::mat4 cubeTransform = translatecube*rotatecube * scalecube;
Matrices.model *= cubeTransform;
MVP = VP * Matrices.model; // MVP = p * V * M

glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

draw3DObject(movblock[i].block);
}
}
if(board[movblock[0].y][movblock[0].x].type==0 || board[movblock[1].y][movblock[1].x].type==0 ||
movblock[0].x<0 || movblock[0].y<0 ||
movblock[1].x<0 || movblock[1].y<0 ||
movblock[0].x>14 || movblock[1].x>14){

  double ctime=glfwGetTime();
  if(ctime-utime>0.05){
    utime=glfwGetTime();
    if(l8f==1)
    movblock[1].movy-=2;
    else if(l8f==2){
      //printf("aaa\n");
    movblock[0].movy-=2;
  }
    else{
    movblock[0].movy-=2;
    movblock[1].movy-=2;
  }
    disable=1;
  }
  if(movblock[0].movy<-15 || movblock[1].movy<-15){
thread(play_audio,"/home/sathwik/Downloads/beep5.mp3").detach();
    //printf("%d yes\n",level);
    set();
  if(level==1)
  level1();
  if(level==2)
  level2();
  if(level==3)
    level3();
  if(level==4)
  level4();
  if(level==5)
  level5();
}
}
if(board[movblock[0].y][movblock[0].x].type==4 && board[movblock[1].y][movblock[1].x].type==4 ){
  double ctime=glfwGetTime();
  disable=1;
  if(ctime-utime>0.05){
    utime=ctime;
    movblock[0].movy-=2;
    movblock[1].movy-=2;
  }
  if(movblock[0].movy<-20){
    points+=currscore;
    set();
    level++;
    if(level==2)
    level2();
    if(level==3){
      level3();
      l3=0;
      r3=18;
    }
    if(level==4){
    //  printf("%d\n",level);
    level4();
      //printf("%d\n",board[6][1].type);
  }
  if(level==5){
    level5();
    r5=-12;
  }
  //if(level==6)
  //exit(0);

}
}
if(level==2){
  Matrices.model = glm::mat4(1.0f);

glm::mat4 translateTriangle1 = glm::translate (glm::vec3(0.0f+(2+1)*6-30, 0.0f, 0.0f+(4+1)*6-30)); // glTranslatef
      glm::mat4 rotateTriangle1 = glm::rotate((float)(0*M_PI/180.0f), glm::vec3(0,0,1));
      glm::mat4 scaleTriangle1 = glm::scale (glm::vec3(1.5f, 0.4f, 1.5f)); // glTranslatef
      // rotate about vector (1,0,0)
      glm::mat4 triangleTransform1 = translateTriangle1 * rotateTriangle1*scaleTriangle1;
      Matrices.model *= triangleTransform1;
      MVP = VP * Matrices.model; // MVP = p * V * M

      glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

      draw3DObject(movblock[2].block);

      Matrices.model = glm::mat4(1.0f);

glm::mat4 translateTriangle2 = glm::translate (glm::vec3(0.0f+(8+1)*6-30, 0.0f, 0.0f+(3+1)*6-30)); // glTranslatef
      glm::mat4 rotateTriangle2 = glm::rotate((float)(0*M_PI/180.0f), glm::vec3(0,0,1));
      glm::mat4 scaleTriangle2 = glm::scale (glm::vec3(1.5f, 0.4f, 1.5f)); // glTranslatef
      // rotate about vector (1,0,0)
      glm::mat4 triangleTransform2 = translateTriangle2 * rotateTriangle2*scaleTriangle2;
      Matrices.model *= triangleTransform2;
      MVP = VP * Matrices.model; // MVP = p * V * M

      glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

      draw3DObject(movblock[3].block);

      if(board[movblock[1].y][movblock[0].x].type==2 ||
         board[movblock[0].y][movblock[0].x].type==2){
      if(board[6][4].type==0 && l2bridge1==0){
        board[6][4].type=1;
        board[6][5].type=1;
        l2tog1=1;
      }
      else if(board[6][4].type==1 && l2bridge1==1){
        board[6][4].type=0;
        board[6][5].type=0;
        l2tog1=0;
      }
      }
      else if(l2tog1==1){
        l2bridge1=1;
      }
      else if(l2tog1==0){
        l2bridge1=0;
      }
      if(board[movblock[0].y][movblock[0].x].type==3 && board[movblock[1].y][movblock[1].x].type==3){
      if(board[6][10].type==0 && l2bridge2==0){
        board[6][10].type=1;
        board[6][11].type=1;
        l2tog2=1;
      }
      else if(board[6][10].type==1  && l2bridge2==1){
        board[6][10].type=0;
        board[6][11].type=0;
        l2tog2=0;
      }
      }
      else if(l2tog2==1){
        l2bridge2=1;
      }
      else if(l2tog2==0){
        l2bridge2=0;
      }

}

if(level==4){
  int a[3]={10,10};
  int b[3]={3,6};
  for(int i=0;i<2;i++){
    if(board[3][10].type==0)
    fragile_y[0]-=4;
    else if(board[6][10].type==0)
    fragile_y[1]-=4;
Matrices.model = glm::mat4(1.0f);

glm::mat4 translateTriangle3 = glm::translate (glm::vec3(0.0f+(a[i]+1)*6-30,0.0f+fragile_y[i], 0.0f+(b[i]+1)*6-30)); // glTranslatef
    glm::mat4 rotateTriangle3 = glm::rotate((float)(0*M_PI/180.0f), glm::vec3(0,0,1));
    glm::mat4 scaleTriangle3 = glm::scale (glm::vec3(1.5f, 0.4f, 1.5f)); // glTranslatef
    // rotate about vector (1,0,0)
    glm::mat4 triangleTransform3 = translateTriangle3 * rotateTriangle3*scaleTriangle3;
    Matrices.model *= triangleTransform3;
    MVP = VP * Matrices.model; // MVP = p * V * M

    glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
    draw3DObject(movblock[4+i].block);
  }
  Matrices.model = glm::mat4(1.0f);

  glm::mat4 translateTriangle4 = glm::translate (glm::vec3(0.0f+14*6-30,0.0f, 0.0f+9*6-30)); // glTranslatef
      glm::mat4 rotateTriangle4 = glm::rotate((float)(0*M_PI/180.0f), glm::vec3(0,0,1));
      glm::mat4 scaleTriangle4 = glm::scale (glm::vec3(1.5f, 0.4f, 1.5f)); // glTranslatef
      // rotate about vector (1,0,0)
      glm::mat4 triangleTransform4= translateTriangle4 * rotateTriangle4*scaleTriangle4;
      Matrices.model *= triangleTransform4;
      MVP = VP * Matrices.model; // MVP = p * V * M

      glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
      draw3DObject(movblock[6].block);

//  board[6][1].type=1;
    //  printf("%d %d\n",movblock[0].y,movblock[0].x);
      if(board[movblock[0].y][movblock[0].x].type==3 && board[movblock[1].y][movblock[1].x].type==3){
      if(board[6][8].type==0 && l4bridge==0){
        board[6][8].type=1;
        board[6][9].type=1;
        board[7][8].type=1;
        board[7][9].type=1;
        l4tog=1;
      }
      else if(board[6][8].type==1  && l4bridge==1){
        board[6][8].type=0;
        board[6][9].type=0;
        board[7][8].type=0;
        board[7][9].type=0;
        l4tog=0;
      }
      }
      else if(l4tog==1){
        l4bridge=1;
      }
      else if(l4tog==0){
        l4bridge=0;
      }
    //  printf("%d %d %d\n",movblock[0].y,movblock[0].x,board[movblock[0].y][movblock[0].x].type );
      if((board[movblock[0].y][movblock[0].x].type==5 && board[movblock[1].y][movblock[1].x].type==5) || flag==1){
        board[movblock[0].y][movblock[0].x].type=0;
        flag=1;
        double ctime1=glfwGetTime();
        if(ctime1-utime1>0.05){
          utime=ctime1;
          movblock[0].movy-=2;
          movblock[1].movy-=2;
          if(movblock[0].y==3)
          fragile_y[0]-=4;
          else
          fragile_y[1]-=4;
          disable=1;
        }
        if(movblock[0].movy<-11){
          set();
          level4();
        }
      }
}
if(level==5){
  Matrices.model = glm::mat4(1.0f);

  glm::mat4 translateTriangle5 = glm::translate (glm::vec3(0.0f+5*6-30,0.0f, 0.0f+6*6-30)); // glTranslatef
      glm::mat4 rotateTriangle5 = glm::rotate((float)(0*M_PI/180.0f), glm::vec3(0,0,1));
      glm::mat4 scaleTriangle5 = glm::scale (glm::vec3(1.5f, 0.4f, 1.5f)); // glTranslatef
      // rotate about vector (1,0,0)
      glm::mat4 triangleTransform5= translateTriangle5 * rotateTriangle5*scaleTriangle5;
      Matrices.model *= triangleTransform5;
      MVP = VP * Matrices.model; // MVP = p * V * M

      glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
      draw3DObject(movblock[7].block);
      if(board[movblock[0].y][movblock[0].x].type==7 && board[movblock[1].y][movblock[1].x].type==7){
        for(int k=0;k<2;k++)
        movblock[k].movx+=36;
        movblock[0].movz+=18;
        movblock[1].movz-=18;
        movblock[0].movy-=6;
        l8f=1;
      }
      if(movblock[0].y==5 && movblock[0].x==11 && movblock[1].y==5 && movblock[1].x==12)
      l8f=0;
      else if(movblock[1].y==5 && movblock[1].x==12){
      l8f=2;
    }
}

stringstream ss1;
if(level==6)
ss1<<level-1;
else
ss1 << level;
string str3 = ss1.str();
int digits = str3.length();
float seg_x=-6.5;

for(int i=0;i<digits;i++){
  set_segments(0,str3[i]);
  for(int j=0;j<9;j++){
    if(segment[j].status){
      glm::mat4 VP1=Matrices.projection1 * (glm::lookAt(glm::vec3(0,0,3), glm::vec3(0,0,0), glm::vec3(0,1,0)));
      Matrices.model = glm::mat4(1.0f);
      glm::mat4 translateSegment = glm::translate (glm::vec3(seg_x+segment[j].x,8+segment[j].y,0));
      //glm::mat4 scalesegment = glm::scale (glm::vec3(0.8f,0.8f,0.8f));
      Matrices.model *= translateSegment;
      MVP = VP1 * Matrices.model;
      glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

      draw3DObject(segment[j].object);
    }
  }
  seg_x+=1;
}


string str1 ="STAGE";
//ss << level;
//string str1 = ss.str();
float seg_x1=-9.5;

for(int i=0;i<5;i++){
  set_segments(0,str1[i]);
  for(int j=0;j<9;j++){
    if(segment[j].status){
      glm::mat4 VP2=Matrices.projection1 * (glm::lookAt(glm::vec3(0,0,3), glm::vec3(0,0,0), glm::vec3(0,1,0)));
      Matrices.model = glm::mat4(1.0f);
      glm::mat4 translateSegment1 = glm::translate (glm::vec3(seg_x1+segment[j].x,8+segment[j].y,0));
      //glm::mat4 scalesegment = glm::scale (glm::vec3(0.8f,0.8f,0.8f));
      Matrices.model *= translateSegment1;
      MVP = VP2 * Matrices.model;
      glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

      draw3DObject(segment[j].object);
    }
  }
  seg_x1+=0.5;
}

string str2 ="MOUES ";
//ss << level;
//string str1 = ss.str();
float  seg_x2=4.5;

for(int i=0;i<7;i++){
  set_segments(0,str2[i]);
  for(int j=0;j<9;j++){
    if(segment[j].status){
      glm::mat4 VP3=Matrices.projection1 * (glm::lookAt(glm::vec3(0,0,3), glm::vec3(0,0,0), glm::vec3(0,1,0)));
      Matrices.model = glm::mat4(1.0f);
      glm::mat4 translateSegment2 = glm::translate (glm::vec3(seg_x2+segment[j].x,8+segment[j].y,0));
      //glm::mat4 scalesegment = glm::scale (glm::vec3(0.8f,0.8f,0.8f));
      Matrices.model *= translateSegment2;
      MVP = VP3 * Matrices.model;
      glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

      draw3DObject(segment[j].object);
    }
  }
  seg_x2+=0.5;
}
if(level==6){
str2 ="YOU WIN";
//ss << level;
//string str1 = ss.str();
seg_x2=-2;

for(int i=0;i<7;i++){
  set_segments(0,str2[i]);
  for(int j=0;j<9;j++){
    if(segment[j].status){
      glm::mat4 VP3=Matrices.projection1 * (glm::lookAt(glm::vec3(0,0,3), glm::vec3(0,0,0), glm::vec3(0,1,0)));
      Matrices.model = glm::mat4(1.0f);
      glm::mat4 translateSegment2 = glm::translate (glm::vec3(seg_x2+segment[j].x,1+segment[j].y,0));
      //glm::mat4 scalesegment = glm::scale (glm::vec3(0.8f,0.8f,0.8f));
      Matrices.model *= translateSegment2;
      MVP = VP3 * Matrices.model;
      glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

      draw3DObject(segment[j].object);
    }
  }
  seg_x2+=0.5;
}
}


int printscore=points+currscore;
stringstream ss2;
ss2 << printscore;
string str4 = ss2.str();
int digits1=str4.length();
seg_x2=7.5;

for(int i=0;i<digits1;i++){
  set_segments(0,str4[i]);
  for(int j=0;j<9;j++){
    if(segment[j].status){
      glm::mat4 VP4=Matrices.projection1 * (glm::lookAt(glm::vec3(0,0,3), glm::vec3(0,0,0), glm::vec3(0,1,0)));
      Matrices.model = glm::mat4(1.0f);
      glm::mat4 translateSegment4 = glm::translate (glm::vec3(seg_x2+segment[j].x,8+segment[j].y,0));
      Matrices.model *= translateSegment4;
      MVP = VP4 * Matrices.model;
      glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

      draw3DObject(segment[j].object);
    }
  }
  seg_x2+=0.5;
}



}
/* Initialise glfw window, I/O callbacks and the renderer to use */
/* Nothing to Edit here */
GLFWwindow* initGLFW (int width, int height)
{
    GLFWwindow* window; // window desciptor/handle

    glfwSetErrorCallback(error_callback);
    if (!glfwInit()) {
//        exit(EXIT_FAILURE);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(width, height, "Sample OpenGL 3.3 Application", NULL, NULL);

    if (!window) {
        glfwTerminate();
//        exit(EXIT_FAILURE);
    }

    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
    glfwSwapInterval( 1 );

    /* --- register callbacks with GLFW --- */

    /* Register function to handle window resizes */
    /* With Retina display on Mac OS X GLFW's FramebufferSize
     is different from WindowSize */
    glfwSetFramebufferSizeCallback(window, reshapeWindow);
    glfwSetWindowSizeCallback(window, reshapeWindow);

    /* Register function to handle window close */
    glfwSetWindowCloseCallback(window, quit);

    /* Register function to handle keyboard input */
    glfwSetKeyCallback(window, keyboard);      // general keyboard input
    glfwSetCharCallback(window, keyboardChar);  // simpler specific character handling

    /* Register function to handle mouse click */
    glfwSetMouseButtonCallback(window, mouseButton);
    glfwSetCursorPosCallback(window, cursor_position);
    glfwSetScrollCallback(window, scroll_callback);
  // mouse button clicks

    return window;
}

/* Initialize the OpenGL rendering properties */
/* Add all the models to be created here */
void initGL (GLFWwindow* window, int width, int height)
{
    /* Objects should be created before any other gl function and shaders */
	// Create the models
	createTriangle (); // Generate the VAO, VBOs, vertices data & copy into the array buffer
	createRectangle ();
  createCube (1); //for tiles of type 1
  createCube (2);//for tiles of type 2
  createCube (3);
  createCube (3);
  createCube (4);
  createCube (5);
  createCube(6);
  createCube(6);
  createCube (5);
  createCube(5);
  level1();
  set();

  //display
  float hh,ww,x_offset,y_offset;
  for(int j=0;j<18;j++){
    int i=j%9;
    x_offset=0;
    if(i>=0 && i<=5){
      hh=0.25;
      ww=0.05;
    }
    else{
      hh=0.05;
      ww=0.35;
    }
    if(i>=0 && i<=2)
      y_offset=0.125;
    if(i>=3 && i<=5)
      y_offset=-0.125;
    if(i==6)
      y_offset=0.25;
    if(i==7)
      y_offset=0;
    if(i==8)
      y_offset=-0.25;
    if(i==0 || i==3)
      x_offset=-0.175;
    if(i==2 || i==5)
      x_offset=0.175;

    createSegment(hh,ww,x_offset,y_offset,0);
  }


	// Create and compile our GLSL program from the shaders
	programID = LoadShaders( "Sample_GL.vert", "Sample_GL.frag" );
	// Get a handle for our "MVP" uniform
	Matrices.MatrixID = glGetUniformLocation(programID, "MVP");


	reshapeWindow (window, width, height);

    // Background color of the scene
	glClearColor (176.0/255.0f, 58.0/255.0f, 46.0/255.0f, 0.0f); // R, G, B, A
	glClearDepth (1.0f);

	glEnable (GL_DEPTH_TEST);
	glDepthFunc (GL_LEQUAL);

    cout << "VENDOR: " << glGetString(GL_VENDOR) << endl;
    cout << "RENDERER: " << glGetString(GL_RENDERER) << endl;
    cout << "VERSION: " << glGetString(GL_VERSION) << endl;
    cout << "GLSL: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
}

int main (int argc, char** argv)
{
	 width = 1400;
	 height = 800;

    GLFWwindow* window = initGLFW(width, height);

	initGL (window, width, height);

    double last_update_time = glfwGetTime(), current_time;

    /* Draw in loop */
    while (!glfwWindowShouldClose(window)) {

        // OpenGL Draw commands
        draw();

        // Swap Frame Buffer in double buffering
        glfwSwapBuffers(window);

        // Poll for Keyboard and mouse events
        glfwPollEvents();

        // Control based on time (Time based transformation like 5 degrees rotation every 0.5s)
        current_time = glfwGetTime(); // Time in seconds
        if ((current_time - last_update_time) >= 0.5) { // atleast 0.5s elapsed since last frame
            // do something every 0.5 seconds ..
            last_update_time = current_time;
        }
    }

    glfwTerminate();
//    exit(EXIT_SUCCESS);
}
