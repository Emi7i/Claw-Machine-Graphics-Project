#version 330 core
out vec4 FragColor;

in vec3 chNormal;  
in vec3 chFragPos;  
in vec2 chUV;
  
uniform vec3 uLightPos; 
uniform vec3 uViewPos; 
uniform vec3 uLightColor;
uniform int uLightType; // 0 = point, 1 = directional
uniform vec3 uAmbientColor; // Separate ambient color

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

    // AMBIENT - completely separate, always use uAmbientColor
    float ambientStrength = 0.8; // Much higher for good visibility
    vec3 ambient = ambientStrength * uAmbientColor;
    
    // POINT LIGHT from top - not directional
    vec3 norm = normalize(chNormal);
    vec3 lightPos = vec3(0.0, 5.0, 0.0); // Fixed position at top
    vec3 lightDir = normalize(lightPos - chFragPos); // Calculate direction from top to fragment
    
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * uLightColor * 0.5; // Reduce intensity by half
    
    // SPECULAR from directional light only
    float specularStrength = 0.5;
    vec3 viewDir = normalize(uViewPos - chFragPos);
    vec3 reflectDir = reflect(-lightDir, norm);  
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * uLightColor;  

    // Combine: ambient (always white) + directional light (colored)
    vec3 result = (ambient + diffuse + specular) * baseColor.rgb;
    FragColor = vec4(result, baseColor.a);
}
