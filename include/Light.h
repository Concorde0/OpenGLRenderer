#ifndef LIGHT_H
#define LIGHT_H

 #include <glm/glm.hpp>
 #include <string>

 class Shader;

class Light {
public:
	Light();
	Light(const glm::vec3& position,
		  const glm::vec3& ambient = glm::vec3(0.2f),
		  const glm::vec3& diffuse = glm::vec3(0.8f),
		  const glm::vec3& specular = glm::vec3(1.0f));

	void SetPosition(const glm::vec3& position);
	void SetAmbient(const glm::vec3& ambient);
	void SetDiffuse(const glm::vec3& diffuse);
	void SetSpecular(const glm::vec3& specular);

	const glm::vec3& GetPosition() const;
	const glm::vec3& GetAmbient() const;
	const glm::vec3& GetDiffuse() const;
	const glm::vec3& GetSpecular() const;

	// 将当前灯光参数上传到 shader，默认写入 uniform: light.xxx
	void Apply(const Shader& shader, const std::string& uniformPrefix = "light") const;

private:
	glm::vec3 m_Position;
	glm::vec3 m_Ambient;
	glm::vec3 m_Diffuse;
	glm::vec3 m_Specular;
};


#endif