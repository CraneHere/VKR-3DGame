#version 450 core

// Входные данные от вершинного шейдера
in vec3 vFragPos;
in vec3 vNormal;
in vec2 vTexCoord;
in vec4 vFragPosLightSpace;

// Directional Light (солнце)
uniform vec3 uLightDir;           // Направление света (нормализованное)
uniform vec3 uLightColor;         // Цвет света (тёплый: 1.0, 0.7, 0.4)
uniform float uLightIntensity;    // Интенсивность

// Hemisphere Ambient (небо/земля)
uniform vec3 uSkyColor;           // Цвет неба (0.6, 0.5, 0.5)
uniform vec3 uGroundColor;        // Цвет земли (0.3, 0.25, 0.2)
uniform float uAmbientIntensity;  // Интенсивность ambient

// Камера
uniform vec3 uViewPos;

// Материал
uniform vec3 uObjectColor;
uniform sampler2D uTexture;
uniform bool uUseTexture;

// Прозрачность (для призрака редактора)
uniform float uAlpha;

// Тени
uniform sampler2D uShadowMap;
uniform bool uShadowsEnabled;

// Туман
uniform vec3 uFogColor;
uniform float uFogDensity;
uniform float uFogHeightFalloff;
uniform bool uFogEnabled;

// Выходной цвет
out vec4 FragColor;

float ShadowCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir)
{
    if (!uShadowsEnabled) return 0.0;
    
    // Перспективное деление
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    
    // Проверка границ
    if (projCoords.z > 1.0 || projCoords.x < 0.0 || projCoords.x > 1.0 ||
        projCoords.y < 0.0 || projCoords.y > 1.0)
        return 0.0;
    
    // Slope-scaled bias для устранения shadow acne
    // Больший bias когда поверхность почти параллельна свету
    float cosTheta = max(dot(normal, lightDir), 0.0);
    float bias = max(0.05 * (1.0 - cosTheta), 0.005);
    
    // PCF (Percentage-Closer Filtering) для мягких теней
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(uShadowMap, 0);
    
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            float pcfDepth = texture(uShadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += projCoords.z - bias > pcfDepth ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0;
    
    return shadow;
}

vec3 ApplyFog(vec3 color, float distance, vec3 worldPos)
{
    if (!uFogEnabled) return color;
    
    // Exponential distance fog
    float fogFactor = 1.0 - exp(-distance * uFogDensity);
    
    // Height-based falloff (туман гуще внизу)
    float heightFog = exp(-max(worldPos.y, 0.0) * uFogHeightFalloff);
    fogFactor *= heightFog;
    
    return mix(color, uFogColor, clamp(fogFactor, 0.0, 1.0));
}

void main()
{
    vec3 norm = normalize(vNormal);
    vec3 viewDir = normalize(uViewPos - vFragPos);
    
    // Hemisphere Ambient (небо/земля)
    // Смешиваем цвет неба и земли в зависимости от направления нормали
    float hemisphereBlend = dot(norm, vec3(0.0, 1.0, 0.0)) * 0.5 + 0.5;
    vec3 ambient = mix(uGroundColor, uSkyColor, hemisphereBlend) * uAmbientIntensity;
    
    // Directional Light (солнце заката)
    vec3 lightDir = normalize(-uLightDir);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * uLightColor * uLightIntensity;
    
    // Specular (Blinn-Phong)
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(norm, halfwayDir), 0.0), 32.0);
    vec3 specular = spec * uLightColor * 0.3 * uLightIntensity;
    
    // Тени
    float shadow = ShadowCalculation(vFragPosLightSpace, norm, lightDir);
    
    // Базовый цвет материала
    vec3 baseColor = uObjectColor;
    if (uUseTexture) {
        baseColor = texture(uTexture, vTexCoord).rgb;
    }
    
    // Финальное освещение
    // Ambient не затеняется, diffuse и specular затеняются
    vec3 lighting = ambient + (1.0 - shadow) * (diffuse + specular);
    vec3 result = lighting * baseColor;
    
    // Применяем туман
    float distanceToCamera = length(uViewPos - vFragPos);
    result = ApplyFog(result, distanceToCamera, vFragPos);
    
    FragColor = vec4(result, uAlpha);
}
