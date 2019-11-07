#pragma once

class IEnvironmentMaterial : public ICloneable<IEnvironmentMaterial>
{
public:
    virtual ~IEnvironmentMaterial() = default;

    virtual RGB getRadiance(const Scene& scene, const Vector3& direction) const = 0;
};
