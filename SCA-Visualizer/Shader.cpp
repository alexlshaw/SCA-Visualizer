#include "Shader.h"

#include <fstream>
#include <sstream>
#include <sys/stat.h>

using std::ifstream;
using std::ios;
using std::ostringstream;

Shader::Shader() : vs(0), fs(0), gs(0), tcs(0), tes(0), handle(0), linked(false), validated(false) { }

Shader::~Shader()
{
	glDeleteProgram(handle);
	if (vs > 0)
	{
		glDeleteShader(vs);
	}
	if (fs > 0)
	{
		glDeleteShader(fs);
	}
	if (gs > 0)
	{
		glDeleteShader(gs);
	}

	if (tes > 0)
	{
		glDeleteShader(tes);
	}
	if (tcs > 0)
	{
		glDeleteShader(tcs);
	}

}

bool Shader::compileShaderFromFile(const char* fileName, GLSLShaderType type)
{
	//open up the file
	if (!fileExists(fileName))
	{
		logString = "Loading Shader: File not found.";
		return false;
	}
	ifstream inFile(fileName, ios::in);
	if (!inFile)
	{
		return false;
	}
	//load the contents to a string
	ostringstream code;
	while (inFile.good())
	{
		int c = inFile.get();
		if (!inFile.eof())
		{
			code << (char)c;
		}
	}
	inFile.close();
	//pass it off to do the actual compilation
	return compileShaderFromString(code.str(), type);
}

bool Shader::compileShaderFromString(const string& source, GLSLShaderType type)
{
	//Create the program if we havn't already got one
	if (handle <= 0)
	{
		handle = glCreateProgram();
		if (handle == 0)
		{
			logString = "Loading Shader: Unable to create shader program.";
			return false;
		}
	}
	//set up the specific shader part
	GLuint shaderHandle = 0;
	switch (type)
	{
	case VERTEX:
		shaderHandle = glCreateShader(GL_VERTEX_SHADER);
		vs = shaderHandle;
		break;
	case FRAGMENT:
		shaderHandle = glCreateShader(GL_FRAGMENT_SHADER);
		fs = shaderHandle;
		break;
	case GEOMETRY:
		shaderHandle = glCreateShader(GL_GEOMETRY_SHADER);
		gs = shaderHandle;
		break;
	case TESS_CONTROL:
		shaderHandle = glCreateShader(GL_TESS_CONTROL_SHADER);
		tcs = shaderHandle;
		break;
	case TESS_EVALUATION:
		shaderHandle = glCreateShader(GL_TESS_EVALUATION_SHADER);
		tes = shaderHandle;
		break;
	default:
		logString = "Loading Shader: Unrecognised shader type.";
		return false;
	}
	//load and compile the shader
	const char* c_code = source.c_str();
	glShaderSource(shaderHandle, 1, &c_code, NULL);
	glCompileShader(shaderHandle);
	//check for errors
	GLint result;
	glGetShaderiv(shaderHandle, GL_COMPILE_STATUS, &result);
	if (result == GL_FALSE)
	{
		//compile failed, log the error and return false
		int length = 0;
		logString = "";
		glGetShaderiv(shaderHandle, GL_INFO_LOG_LENGTH, &length);
		if (length > 0)
		{
			char* c_log = new char[length];
			int written = 0;
			glGetShaderInfoLog(shaderHandle, length, &written, c_log);
			logString = c_log;
			delete[] c_log;
		}
		return false;
	}
	else
	{
		//compile succeeded, attach shader and return true
		glAttachShader(handle, shaderHandle);
		return true;
	}
}

bool Shader::link()
{
	if (!linked)
	{
		if (handle <= 0)
		{
			logString = "Loading Shader: Linking failed. No shader handle.";
			return false;
		}
		glLinkProgram(handle);
		//check that everything went well, if not, log an error and return false;
		GLint status = 0;
		glGetProgramiv(handle, GL_LINK_STATUS, &status);
		if (status == GL_FALSE)
		{
			int length = 0;
			logString = "";
			glGetProgramiv(handle, GL_INFO_LOG_LENGTH, &length);
			if (length > 0)
			{
				char* c_log = new char[length];
				int written = 0;
				glGetProgramInfoLog(handle, length, &written, c_log);
				logString = c_log;
				delete[] c_log;
			}
		}
		linked = true;
	}
	//everything succeeded, return linked (should be true)
	return linked;
}

bool Shader::validate()
{
	if (!linked)
	{
		logString = "Loading Shader: Validation failed, shader is not linked.";
		return false;
	}
	GLint status;
	glValidateProgram(handle);
	glGetProgramiv(handle, GL_VALIDATE_STATUS, &status);
	//validation failed, log it and return false
	if (status == GL_FALSE)
	{
		int length = 0;
		logString = "";
		glGetProgramiv(handle, GL_INFO_LOG_LENGTH, &length);
		if (length > 0)
		{
			char* c_log = new char[length];
			int written = 0;
			glGetProgramInfoLog(handle, length, &written, c_log);
			logString = c_log;
			delete[] c_log;
		}
		else
		{
			logString = "Shader validation failed, but OpenGL has nothing to say about why.\n";
		}
		return false;
	}
	//validation passed, return true
	validated = true;
	return true;
}

bool Shader::linkAndValidate()
{
	if (!link())
	{
		printf("Failed to link shader\n");
		printf(getLog().c_str());
		return false;
	}
	if (!validate())
	{
		printf("Failed to validate shader\n");
		printf(getLog().c_str());
		return false;
	}
	return true;
}

void Shader::use()
{
	if (handle > 0 && linked)
	{
		glUseProgram(handle);
	}
}

string Shader::getLog()
{
	return logString;
}

GLuint Shader::getHandle()
{
	return handle;
}

bool Shader::isLinked()
{
	return linked;
}

void Shader::bindAttribLocation(GLuint location, const char* name)
{
	glBindAttribLocation(handle, location, name);
}

void Shader::bindFragDataLocation(GLuint location, const char* name)
{
	glBindFragDataLocation(handle, location, name);
}

int Shader::getUniformLocation(const char* name)
{
	int result = glGetUniformLocation(handle, name);
	if (glGetError() != GL_NO_ERROR)
	{
		printf("Error occured retrieving uniform location: ");
		printf(name);
		printf("\n");
	}
	return result;
}

void Shader::setUniform(GLint location, float x, float y)
{
	if (location >= 0)
	{
		glUniform2f(location, x, y);
	}
	else
	{
		printf("Uniform: %d not found.\n", location);
	}
}

void Shader::setUniform(GLint location, float x, float y, float z)
{
	if (location >= 0)
	{
		glUniform3f(location, x, y, z);
	}
	else
	{
		printf("Uniform: %d not found.\n", location);
	}
}

void Shader::setUniform(GLint location, const vec2& v)
{
	if (location >= 0)
	{
		glUniform2f(location, v.x, v.y);
	}
	else
	{
		printf("Uniform: %d not found.\n", location);
	}
}

void Shader::setUniform(GLint location, const vec3& v)
{
	if (location >= 0)
	{
		glUniform3f(location, v.x, v.y, v.z);
	}
	else
	{
		printf("Uniform: %d not found.\n", location);
	}
}

void Shader::setUniform(GLint location, const vec4& v)
{
	if (location >= 0)
	{
		glUniform4f(location, v.x, v.y, v.z, v.w);
	}
	else
	{
		printf("Uniform: %d not found.\n", location);
	}
}

void Shader::setUniform(GLint location, const mat4& m)
{
	if (location >= 0)
	{
		glUniformMatrix4fv(location, 1, GL_FALSE, &m[0][0]);
	}
	else
	{
		printf("Uniform: %d not found.\n", location);
	}
}

void Shader::setUniform(GLint location, const mat3& m)
{
	if (location >= 0)
	{
		glUniformMatrix3fv(location, 1, GL_FALSE, &m[0][0]);
	}
	else
	{
		printf("Uniform: %d not found.\n", location);
	}
}

void Shader::setUniform(GLint location, float val)
{
	if (location >= 0)
	{
		glUniform1f(location, val);
	}
	else
	{
		printf("Uniform: %d not found.\n", location);
	}
}

void Shader::setUniform(GLint location, int val)
{
	if (location >= 0)
	{
		glUniform1i(location, val);
	}
	else
	{
		printf("Uniform: %d not found.\n", location);
	}
}

void Shader::setUniform(GLint location, bool val)
{
	if (location >= 0)
	{
		glUniform1i(location, val);
	}
	else
	{
		printf("Uniform: %d not found.\n", location);
	}
}

void Shader::printActiveUniforms()
{
	GLint numUniforms, size, location, maxLength;
	GLchar* name;
	GLsizei written;
	GLenum type;

	glGetProgramiv(handle, GL_ACTIVE_UNIFORM_MAX_LENGTH, &maxLength);
	glGetProgramiv(handle, GL_ACTIVE_UNIFORMS, &numUniforms);
	//allocate memory for the uniform name
	name = (GLchar*)malloc(maxLength);
	printf(" Location | Name\n");
	printf("------------------------------------------------\n");
	for (int i = 0; i < numUniforms; i++)
	{
		glGetActiveUniform(handle, i, maxLength, &written, &size, &type, name);
		location = glGetUniformLocation(handle, name);
		printf(" %-8d | %s\n", location, name);
	}
	//free memory allocated for the uniform name
	free(name);
}

void Shader::printActiveAttribs()
{
	GLint written, size, location, maxLength, numAttribs;
	GLenum type;
	GLchar* name;

	glGetProgramiv(handle, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &maxLength);
	glGetProgramiv(handle, GL_ACTIVE_ATTRIBUTES, &numAttribs);
	//allocate memory for the attribute name
	name = (GLchar*)malloc(maxLength);
	printf(" Index | Name\n");
	printf("------------------------------------------------\n");
	for (int i = 0; i < numAttribs; i++)
	{
		glGetActiveAttrib(handle, i, maxLength, &written, &size, &type, name);
		location = glGetAttribLocation(handle, name);
		printf(" %-5d | %s\n", location, name);
	}
	//free memory allocated for the attribute name
	free(name);
}

bool Shader::fileExists(const string& fileName)
{
	struct stat info;
	int retVal = -1;
	retVal = stat(fileName.c_str(), &info);
	return retVal == 0;
}