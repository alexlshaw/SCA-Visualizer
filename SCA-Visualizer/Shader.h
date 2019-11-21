#pragma once

#include "glad/glad.h"
#include "glm/glm.hpp"
#include <string>

using std::string;
using glm::vec2;
using glm::vec3;
using glm::vec4;
using glm::mat4;
using glm::mat3;

enum GLSLShaderType {
	VERTEX,
	FRAGMENT,
	GEOMETRY,
	TESS_CONTROL,
	TESS_EVALUATION
};

class Shader
{
private:
	int vs;
	int fs;
	int gs;
	int tcs;
	int tes;
	GLuint handle;
	bool linked, validated;
	string logString;
	bool fileExists(const string& fileName);	//While files are handled in a bunch of places, this shader code is used in many applications, so we keep this self contained
public:
	Shader();
	~Shader();

	bool compileShaderFromFile(const char* fileName, GLSLShaderType type);
	bool compileShaderFromString(const string& source, GLSLShaderType type);
	bool link();
	bool validate();
	bool linkAndValidate();
	void use();

	string getLog();

	GLuint getHandle();
	bool isLinked();

	void bindAttribLocation(GLuint location, const char* name);
	void bindFragDataLocation(GLuint location, const char* name);

	int getUniformLocation(const char* name);
	void setUniform(GLint location, float x, float y);
	void setUniform(GLint location, float x, float y, float z);
	void setUniform(GLint location, const vec2& v);
	void setUniform(GLint location, const vec3& v);
	void setUniform(GLint location, const vec4& v);
	void setUniform(GLint location, const mat4& m);
	void setUniform(GLint location, const mat3& m);
	void setUniform(GLint location, float val);
	void setUniform(GLint location, int val);
	void setUniform(GLint location, bool val);
	void printActiveUniforms();
	void printActiveAttribs();
};