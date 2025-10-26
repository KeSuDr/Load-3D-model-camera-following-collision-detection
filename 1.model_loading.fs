#version 330 core
out vec4 FragColor;

in vec2 TexCoords;
in vec3 Normal;
in vec3 FragPos;

// Material properties (we'll send these from model.h)
uniform vec3 material_diffuse_color;
uniform vec3 material_specular_color;
uniform float material_shininess;
uniform int has_diffuse_texture; // Using int for bool (1=true, 0=false)
uniform sampler2D texture_diffuse1; // Keep for models that do use textures

// Light properties
uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 lightColor; // You'll need to add this uniform in main.cpp

void main()
{
    // 1. Get Base Color
    vec3 color;
    if(has_diffuse_texture == 1)
        color = texture(texture_diffuse1, TexCoords).rgb;
    else
        color = material_diffuse_color; // Use the Kd color from the MTL file

    // 2. Ambient
    // We'll use a simple 0.1 ambient factor
    vec3 ambient = 0.1 * color; 

    // 3. Diffuse
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * color; // Using lightColor * diff * color is also common

    // 4. Specular (Now uses Ks and Ns from MTL)
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material_shininess);
    vec3 specular = material_specular_color * spec; 

    FragColor = vec4(ambient + diffuse + specular, 1.0);
}