#version 330 core
out vec4 FragColor;

in vec3 chNormal;  
in vec3 chFragPos;  
in vec2 chUV;
  
uniform vec3 uLightPos; 
uniform vec3 uViewPos; 
uniform vec3 uLightColor;

uniform vec3 uDiffuseColor;  // Material color
uniform int uHasTexture; 
uniform float uOpacity;   

uniform sampler2D uDiffMap1;

void main()
{    
    // Get base color with alpha
    vec4 baseColor;
    if(uHasTexture == 1) {
        baseColor = texture(uDiffMap1, chUV);
    } else {
        baseColor = vec4(uDiffuseColor, 1.0);
    }
    
    // Apply material opacity
    baseColor.a *= uOpacity;
    
    // Discard fully transparent pixels
    if(baseColor.a < 0.1)
        discard;

    // ambient
    float ambientStrength = 0.1;
    vec3 ambient = ambientStrength * uLightColor;
    
    // diffuse 
    vec3 norm = normalize(chNormal);
    vec3 lightDir = normalize(uLightPos - chFragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * uLightColor;
    
    // specular
    float specularStrength = 0.5;
    vec3 viewDir = normalize(uViewPos - chFragPos);
    vec3 reflectDir = reflect(-lightDir, norm);  
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * uLightColor;  

    // Combine lighting with base color (keep alpha)
    vec3 result = (ambient + diffuse + specular) * baseColor.rgb;
    FragColor = vec4(result, baseColor.a);
}