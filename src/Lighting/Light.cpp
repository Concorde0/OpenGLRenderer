#include "../../include/Light.h"

#include "../../include/Shader.h"

Light::Light()
	: m_Position(1.2f, 1.0f, 2.0f),
	  m_Ambient(0.2f, 0.2f, 0.2f),
	  m_Diffuse(0.8f, 0.8f, 0.8f),
	  m_Specular(1.0f, 1.0f, 1.0f)
{
}

Light::Light(const glm::vec3& position,
			 const glm::vec3& ambient,
			 const glm::vec3& diffuse,
			 const glm::vec3& specular)
	: m_Position(position),
	  m_Ambient(ambient),
	  m_Diffuse(diffuse),
	  m_Specular(specular)
{
}

void Light::SetPosition(const glm::vec3& position)
{
	m_Position = position;
}

void Light::SetAmbient(const glm::vec3& ambient)
{
	m_Ambient = ambient;
}

void Light::SetDiffuse(const glm::vec3& diffuse)
{
	m_Diffuse = diffuse;
}

void Light::SetSpecular(const glm::vec3& specular)
{
	m_Specular = specular;
}

const glm::vec3& Light::GetPosition() const
{
	return m_Position;
}

const glm::vec3& Light::GetAmbient() const
{
	return m_Ambient;
}

const glm::vec3& Light::GetDiffuse() const
{
	return m_Diffuse;
}

const glm::vec3& Light::GetSpecular() const
{
	return m_Specular;
}

void Light::Apply(const Shader& shader, const std::string& uniformPrefix) const
{
	shader.SetVec3(uniformPrefix + ".position", m_Position);
	shader.SetVec3(uniformPrefix + ".ambient", m_Ambient);
	shader.SetVec3(uniformPrefix + ".diffuse", m_Diffuse);
	shader.SetVec3(uniformPrefix + ".specular", m_Specular);
}

