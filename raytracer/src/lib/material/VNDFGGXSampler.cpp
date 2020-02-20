#include <math/Constants.h>
#include "VNDFGGXSampler.h"
#include "math/OrthonormalBasis.h"
#include "math/Vector2.h"
#include "math/FastRandom.h"

Vector2 sampleP22(const double theta_i)
{
    double U1 = Rand::unitDouble();
    double U2 = Rand::unitDouble();
    // special case (normal incidence)
    if(theta_i < 0.0001)
    {
        const double r = sqrt(U1/(1-U1));
        const double phi = 6.28318530718 * U2;
        return Vector2(r * cos(phi), r * sin(phi));
    }
    // precomputations
    const double tan_theta_i = tan(theta_i);
    const double a = 1 / (tan_theta_i);
    const double G1 = 2 / (1 + sqrt(1.0+1.0/(a*a)));
    // sample slope_x
    const double A = 2.0*U1/G1 - 1.0;
    const double tmp = 1.0 / (A*A-1.0);
    const double B = tan_theta_i;
    const double D = sqrt(B*B*tmp*tmp - (A*A-B*B)*tmp);
    const double slope_x_1 = B*tmp - D;
    const double slope_x_2 = B*tmp + D;
    double slope_x = (A < 0 || slope_x_2 > 1.0/tan_theta_i) ? slope_x_1 : slope_x_2;
    // sample slope_y
    double S;
    if(U2 > 0.5)
    {
        S = 1.0;
        U2 = 2.0*(U2-0.5);
    }
    else
    {
        S = -1.0;
        U2 = 2.0*(0.5-U2);
    }
    const double z = (U2*(U2*(U2*0.27385-0.73369)+0.46341)) / (U2*(U2*(U2*0.093073+0.309420)-1.000000)+0.597999);
    double slope_y = S * z * sqrt(1.0+slope_x*slope_x);
    return Vector2(slope_x, slope_y);
}


Vector3 VNDFGGXSampler::sample(const Vector3& smoothNormal, const Vector3& worldIncoming, float roughness)
{
    // Transform incoming vector to smooth normal space
    OrthonormalBasis basis(smoothNormal);
    Vector3 incoming = basis.applyBasisTo(worldIncoming);
    incoming.normalize();

    // Stretch incoming
    Vector3 incomingStretched(incoming.x() * roughness, incoming.y() * roughness, incoming.z());
    incomingStretched.normalize();

    // Transform to polar coordinates
    float theta = 0.0, phi = 0.0;
    if(incomingStretched.z() < 0.99999)
    {
        theta = std::acos(incomingStretched.z());
        phi = std::atan2(incomingStretched.y(), incomingStretched.x());
    }

    // Sample P22
    Vector2 slope = sampleP22(theta);

    // Rotate
    Vector2 rotatedSlope(
            std::cos(phi) * slope.x() - std::sin(phi) * slope.y(),
            std::sin(phi) * slope.x() + std::cos(phi) * slope.y()
        );

    // Unstretch
    rotatedSlope *= roughness;

    // Compute normal
    Vector3 microNormal(-rotatedSlope.x(), -rotatedSlope.y(), 1.0f);
    microNormal.normalize();

    // Transform back from normal to object space
    Vector3 result = basis.invertApplyBasisTo(microNormal);
    return result;
}
