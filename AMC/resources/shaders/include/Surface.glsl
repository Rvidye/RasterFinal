struct Surface {
    vec3 Albedo;
    vec3 Emissive;
    vec3 Normal;
    float Metallic;
    float Roughness;
    float Alpha;
};

Surface GetDefaultSurface()
{
    Surface surface;

    surface.Albedo = vec3(1.0);
    surface.Alpha = 1.0;

    surface.Normal = vec3(0.0);
    surface.Emissive = vec3(0.0);
    surface.Metallic = 0.0;
    surface.Roughness = 0.0;    
    return surface;
}