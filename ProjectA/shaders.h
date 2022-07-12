

//vertex shader w/ two rotation matricies stored, plus color grading
const char* vertex_shader = "\
#version 330\n\
layout(location = 0) in vec3 position;\
uniform vec3 offset;\
uniform mat4 perspective;\
uniform vec2 angle;\
uniform vec4 color;\
uniform bool isGradient;\
smooth out vec4 theColor;\
void main(){\
  mat4 xRMatrix = mat4(cos(angle.x), 0.0, sin(angle.x), 0.0,\
                        0.0, 1.0, 0.0, 0.0,\
                        -sin(angle.x), 0.0, cos(angle.x), 0.0,\
                        0.0, 0.0, 0.0, 1.0);\
  mat4 yRMatrix = mat4(1.0, 0.0, 0.0, 0.0,\
                  0.0, cos(angle.y), -sin(angle.y), 0.0,\
                  0.0, sin(angle.y), cos(angle.y), 0.0,\
                  0.0, 0.0, 0.0, 1.0);\
  vec4 rotatedPosition = vec4( position.xyz, 1.0f ) * xRMatrix * yRMatrix;\
  vec4 cameraPos = rotatedPosition + vec4(offset.x, offset.y, offset.z, 0.0);\
  gl_Position = perspective * cameraPos;\
  if (isGradient) theColor = mix(vec4(color.x, color.y, color.z, color.a), vec4(color.x + (1-color.x)/2, color.y + (1-color.y)/2, color.z + (1-color.z)/2, color.a), abs(position.y) / 100);\
  else theColor = color;\
}";


// defualt fragment shader
const char* fragment_shader = "\
#version 330\n\
smooth in vec4 theColor;\
out vec4 outputColor;\
void main(){\
  outputColor = theColor;\
}";

