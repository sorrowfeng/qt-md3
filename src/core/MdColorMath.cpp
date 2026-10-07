#include "MdColorMath.h"

#include <algorithm>
#include <cmath>
#include <unordered_map>

namespace md {

namespace {

// SPDX-License-Identifier: Apache-2.0
// Transcribed from material-color-utilities cpp/.

constexpr double kPi = 3.141592653589793;
constexpr double kWhitePointD65[3] = {95.047, 100.0, 108.883};

/// ViewingConditions, exactly as in cpp/cam/viewing_conditions.h.
struct ViewingConditions {
    double adaptingLuminance = 0.0;
    double backgroundLstar = 0.0;
    double surround = 0.0;
    bool discountingIlluminant = false;
    double backgroundYToWhitePointY = 0.0;
    double aw = 0.0;
    double nbb = 0.0;
    double ncb = 0.0;
    double c = 0.0;
    double nC = 0.0;
    double fl = 0.0;
    double flRoot = 0.0;
    double z = 0.0;
    double whitePoint[3] = {0.0, 0.0, 0.0};
    double rgbD[3] = {0.0, 0.0, 0.0};
};

// kDefaultViewingConditions, verbatim from the same header.
const ViewingConditions kDefaultViewingConditions = {
    11.725676537, 50.000000000, 2.000000000, false, 0.184186503, 29.981000900,
    1.016919255,  1.016919255,  0.689999998, 1.000000000, 0.388481468,
    0.789482653,  1.909169555,  {95.047, 100.0, 108.883},
    {1.021177769, 0.986307740,  0.933960497},
};

double lerp(double start, double stop, double amount)
{
    return (1.0 - amount) * start + amount * stop;
}

int signum(double num)
{
    if (num < 0.0) {
        return -1;
    }
    if (num == 0.0) {
        return 0;
    }
    return 1;
}

MdVec3 matrixMultiply(MdVec3 input, const double matrix[3][3])
{
    return {
        input.a * matrix[0][0] + input.b * matrix[0][1] + input.c * matrix[0][2],
        input.a * matrix[1][0] + input.b * matrix[1][1] + input.c * matrix[1][2],
        input.a * matrix[2][0] + input.b * matrix[2][1] + input.c * matrix[2][2],
    };
}

MdCam16 camFromXyzAndViewingConditionsImpl(double x, double y, double z,
                                          const ViewingConditions &vc){
    const double rC = 0.401288 * x + 0.650173 * y - 0.051461 * z;
    const double gC = -0.250268 * x + 1.204414 * y + 0.045854 * z;
    const double bC = -0.002079 * x + 0.048952 * y + 0.953127 * z;

    const double rD = vc.rgbD[0] * rC;
    const double gD = vc.rgbD[1] * gC;
    const double bD = vc.rgbD[2] * bC;

    const double rAf = std::pow(vc.fl * std::fabs(rD) / 100.0, 0.42);
    const double gAf = std::pow(vc.fl * std::fabs(gD) / 100.0, 0.42);
    const double bAf = std::pow(vc.fl * std::fabs(bD) / 100.0, 0.42);

    const double rA = signum(rD) * 400.0 * rAf / (rAf + 27.13);
    const double gA = signum(gD) * 400.0 * gAf / (gAf + 27.13);
    const double bA = signum(bD) * 400.0 * bAf / (bAf + 27.13);

    const double a = (11.0 * rA + -12.0 * gA + bA) / 11.0;
    const double b = (rA + gA - 2.0 * bA) / 9.0;
    const double u = (20.0 * rA + 20.0 * gA + 21.0 * bA) / 20.0;
    const double p2 = (40.0 * rA + 20.0 * gA + bA) / 20.0;

    const double degrees = std::atan2(b, a) * 180.0 / kPi;
    const double hue = MdColorMath::sanitizeDegreesDouble(degrees);
    const double hueRadians = hue * kPi / 180.0;
    const double ac = p2 * vc.nbb;

    const double j = 100.0 * std::pow(ac / vc.aw, vc.c * vc.z);
    const double q = (4.0 / vc.c) * std::sqrt(j / 100.0) * (vc.aw + 4.0) * vc.flRoot;
    const double huePrime = hue < 20.14 ? hue + 360.0 : hue;
    const double eHue = 0.25 * (std::cos(huePrime * kPi / 180.0 + 2.0) + 3.8);
    const double p1 = 50000.0 / 13.0 * eHue * vc.nC * vc.ncb;
    const double t = p1 * std::sqrt(a * a + b * b) / (u + 0.305);
    const double alpha = std::pow(t, 0.9)
                         * std::pow(1.64 - std::pow(0.29, vc.backgroundYToWhitePointY), 0.73);
    const double c = alpha * std::sqrt(j / 100.0);
    const double m = c * vc.flRoot;
    const double s = 50.0 * std::sqrt((alpha * vc.c) / (vc.aw + 4.0));
    const double jstar = (1.0 + 100.0 * 0.007) * j / (1.0 + 0.007 * j);
    const double mstar = 1.0 / 0.0228 * std::log(1.0 + 0.0228 * m);
    const double astar = mstar * std::cos(hueRadians);
    const double bstar = mstar * std::sin(hueRadians);

    return {hue, c, j, q, m, s, jstar, astar, bstar};
}

// ---------------------------------------------------------------------------
// HCT solver (cpp/cam/hct_solver.cc), transcribed.
// ---------------------------------------------------------------------------

constexpr double kScaledDiscountFromLinrgb[3][3] = {
    {0.001200833568784504, 0.002389694492170889, 0.0002795742885861124},
    {0.0005891086651375999, 0.0029785502573438758, 0.0003270666104008398},
    {0.00010146692491640572, 0.0005364214359186694, 0.0032979401770712076},
};

constexpr double kLinrgbFromScaledDiscount[3][3] = {
    {1373.2198709594231, -1100.4251190754821, -7.278681089101213},
    {-271.815969077903, 559.6580465940733, -32.46047482791194},
    {1.9622899599665666, -57.173814538844006, 308.7233197812385},
};

constexpr double kYFromLinrgb[3] = {0.2126, 0.7152, 0.0722};

// clang-format off
constexpr double kCriticalPlanes[255] = {
    0.015176349177441876, 0.045529047532325624, 0.07588174588720938,
    0.10623444424209313,  0.13658714259697685,  0.16693984095186062,
    0.19729253930674434,  0.2276452376616281,   0.2579979360165119,
    0.28835063437139563,  0.3188300904430532,   0.350925934958123,
    0.3848314933096426,   0.42057480301049466,  0.458183274052838,
    0.4976837250274023,   0.5391024159806381,   0.5824650784040898,
    0.6277969426914107,   0.6751227633498623,   0.7244668422128921,
    0.775853049866786,    0.829304845476233,    0.8848452951698498,
    0.942497089126609,    1.0022825574869039,   1.0642236851973577,
    1.1283421258858297,   1.1946592148522128,   1.2631959812511864,
    1.3339731595349034,   1.407011200216447,    1.4823302800086415,
    1.5599503113873272,   1.6398909516233677,   1.7221716113234105,
    1.8068114625156377,   1.8938294463134073,   1.9832442801866852,
    2.075074464868551,    2.1693382909216234,   2.2660538449872063,
    2.36523901573795,     2.4669114995532007,   2.5710888059345764,
    2.6777882626779785,   2.7870270208169257,   2.898822059350997,
    3.0131901897720907,   3.1301480604002863,   3.2497121605402226,
    3.3718988244681087,   3.4967242352587946,   3.624204428461639,
    3.754355295633311,    3.887192587735158,    4.022731918402185,
    4.160988767090289,    4.301978482107941,    4.445716283538092,
    4.592217266055746,    4.741496401646282,    4.893568542229298,
    5.048448422192488,    5.20615066083972,     5.3666897647573375,
    5.5300801301023865,   5.696336044816294,    5.865471690767354,
    6.037501145825082,    6.212438385869475,    6.390297286737924,
    6.571091626112461,    6.7548350853498045,   6.941541251256611,
    7.131223617812143,    7.323895587840543,    7.5195704746346665,
    7.7182615035334345,   7.919981813454504,    8.124744458384042,
    8.332562408825165,    8.543448553206703,    8.757415699253682,
    8.974476575321063,    9.194643831691977,    9.417930041841839,
    9.644347703669503,    9.873909240696694,    10.106627003236781,
    10.342513269534024,   10.58158024687427,    10.8238400726681,
    11.069304815507364,   11.317986476196008,   11.569896988756009,
    11.825048221409341,   12.083451977536606,   12.345119996613247,
    12.610063955123938,   12.878295467455942,   13.149826086772048,
    13.42466730586372,    13.702830557985108,   13.984327217668513,
    14.269168601521828,   14.55736596900856,    14.848930523210871,
    15.143873411576273,   15.44220572664832,    15.743938506781891,
    16.04908273684337,    16.35764934889634,    16.66964922287304,
    16.985093187232053,   17.30399201960269,    17.62635644741625,
    17.95219714852476,    18.281524751807332,   18.614349837764564,
    18.95068293910138,    19.290534541298456,   19.633915083172692,
    19.98083495742689,    20.331304511189067,   20.685334046541502,
    21.042933821039977,   21.404114048223256,   21.76888489811322,
    22.137256497705877,   22.50923893145328,    22.884842241736916,
    23.264076429332462,   23.6469514538663,     24.033477234264016,
    24.42366364919083,    24.817520537484558,   25.21505769858089,
    25.61628489293138,    26.021211842414342,   26.429848230738664,
    26.842203703840827,   27.258287870275353,   27.678110301598522,
    28.10168053274597,    28.529008062403893,   28.96010235337422,
    29.39497283293396,    29.83362889318845,    30.276079891419332,
    30.722335150426627,   31.172403958865512,   31.62629557157785,
    32.08401920991837,    32.54558406207592,    33.010999283389665,
    33.4802739966603,     33.953417292456834,   34.430438229418264,
    34.911345834551085,   35.39614910352207,    35.88485700094671,
    36.37747846067349,    36.87402238606382,    37.37449765026789,
    37.87891309649659,    38.38727753828926,    38.89959975977785,
    39.41588851594697,    39.93615253289054,    40.460400508064545,
    40.98864111053629,    41.520882981230194,   42.05713473317016,
    42.597404951718396,   43.141702194811224,   43.6900349931913,
    44.24241185063697,    44.798841244188324,   45.35933162437017,
    45.92389141541209,    46.49252901546552,    47.065252796817916,
    47.64207110610409,    48.22299226451468,    48.808024568002054,
    49.3971762874833,     49.9904556690408,     50.587870934119984,
    51.189430279724725,   51.79514187861014,    52.40501387947288,
    53.0190544071392,     53.637271562750364,   54.259673423945976,
    54.88626804504493,    55.517063457223934,   56.15206766869424,
    56.79128866487574,    57.43473440856916,    58.08241284012621,
    58.734331877617365,   59.39049941699807,    60.05092333227251,
    60.715611475655585,   61.38457167773311,    62.057811747619894,
    62.7353394731159,     63.417162620860914,   64.10328893648692,
    64.79372614476921,    65.48848194977529,    66.18756403501224,
    66.89098006357258,    67.59873767827808,    68.31084450182222,
    69.02730813691093,    69.74813616640164,    70.47333615344107,
    71.20291564160104,    71.93688215501312,    72.67524319850172,
    73.41800625771542,    74.16517879925733,    74.9167682708136,
    75.67278210128072,    76.43322770089146,    77.1981124613393,
    77.96744375590167,    78.74122893956174,    79.51947534912904,
    80.30219030335869,    81.08938110306934,    81.88105503125999,
    82.67721935322541,    83.4778813166706,     84.28304815182372,
    85.09272707154808,    85.90692527145302,    86.72564993000343,
    87.54890820862819,    88.3767072518277,     89.2090541872801,
    90.04595612594655,    90.88742016217518,    91.73345337380438,
    92.58406282226491,    93.43925555268066,    94.29903859396902,
    95.16341895893969,    96.03240364439274,    96.9059996312159,
    97.78421388448044,    98.6670533535366,     99.55452497210776,
};
// clang-format on

double sanitizeRadians(double angle)
{
    return std::fmod(angle + kPi * 8, kPi * 2);
}

double trueDelinearized(double rgbComponent)
{
    const double normalized = rgbComponent / 100.0;
    double delinearized = 0.0;
    if (normalized <= 0.0031308) {
        delinearized = normalized * 12.92;
    } else {
        delinearized = 1.055 * std::pow(normalized, 1.0 / 2.4) - 0.055;
    }
    return delinearized * 255.0;
}

double chromaticAdaptation(double component)
{
    const double af = std::pow(std::fabs(component), 0.42);
    return signum(component) * 400.0 * af / (af + 27.13);
}

double hueOf(MdVec3 linrgb)
{
    const MdVec3 scaledDiscount = matrixMultiply(linrgb, kScaledDiscountFromLinrgb);
    const double rA = chromaticAdaptation(scaledDiscount.a);
    const double gA = chromaticAdaptation(scaledDiscount.b);
    const double bA = chromaticAdaptation(scaledDiscount.c);
    const double a = (11.0 * rA + -12.0 * gA + bA) / 11.0;
    const double b = (rA + gA - 2.0 * bA) / 9.0;
    return std::atan2(b, a);
}

bool areInCyclicOrder(double a, double b, double c)
{
    const double deltaAB = sanitizeRadians(b - a);
    const double deltaAC = sanitizeRadians(c - a);
    return deltaAB < deltaAC;
}

double intercept(double source, double mid, double target)
{
    return (mid - source) / (target - source);
}

MdVec3 lerpPoint(MdVec3 source, double t, MdVec3 target)
{
    return {
        source.a + (target.a - source.a) * t,
        source.b + (target.b - source.b) * t,
        source.c + (target.c - source.c) * t,
    };
}

double getAxis(MdVec3 vector, int axis)
{
    switch (axis) {
    case 0:
        return vector.a;
    case 1:
        return vector.b;
    case 2:
        return vector.c;
    default:
        return -1.0;
    }
}

MdVec3 setCoordinate(MdVec3 source, double coordinate, MdVec3 target, int axis)
{
    const double t = intercept(getAxis(source, axis), coordinate, getAxis(target, axis));
    return lerpPoint(source, t, target);
}

bool isBounded(double x)
{
    return x >= 0.0 && x <= 100.0;
}

MdVec3 nthVertex(double y, int n)
{
    const double kR = kYFromLinrgb[0];
    const double kG = kYFromLinrgb[1];
    const double kB = kYFromLinrgb[2];
    const double coordA = n % 4 <= 1 ? 0.0 : 100.0;
    const double coordB = n % 2 == 0 ? 0.0 : 100.0;
    if (n < 4) {
        const double g = coordA;
        const double b = coordB;
        const double r = (y - g * kG - b * kB) / kR;
        if (isBounded(r)) {
            return {r, g, b};
        }
        return {-1.0, -1.0, -1.0};
    }
    if (n < 8) {
        const double b = coordA;
        const double r = coordB;
        const double g = (y - r * kR - b * kB) / kG;
        if (isBounded(g)) {
            return {r, g, b};
        }
        return {-1.0, -1.0, -1.0};
    }
    const double r = coordA;
    const double g = coordB;
    const double b = (y - r * kR - g * kG) / kB;
    if (isBounded(b)) {
        return {r, g, b};
    }
    return {-1.0, -1.0, -1.0};
}

void bisectToSegment(double y, double targetHue, MdVec3 out[2])
{
    MdVec3 left{-1.0, -1.0, -1.0};
    MdVec3 right = left;
    double leftHue = 0.0;
    double rightHue = 0.0;
    bool initialized = false;
    bool uncut = true;
    for (int n = 0; n < 12; ++n) {
        const MdVec3 mid = nthVertex(y, n);
        if (mid.a < 0) {
            continue;
        }
        const double midHue = hueOf(mid);
        if (!initialized) {
            left = mid;
            right = mid;
            leftHue = midHue;
            rightHue = midHue;
            initialized = true;
            continue;
        }
        if (uncut || areInCyclicOrder(leftHue, midHue, rightHue)) {
            uncut = false;
            if (areInCyclicOrder(leftHue, targetHue, midHue)) {
                right = mid;
                rightHue = midHue;
            } else {
                left = mid;
                leftHue = midHue;
            }
        }
    }
    out[0] = left;
    out[1] = right;
}

MdVec3 midpoint(MdVec3 a, MdVec3 b)
{
    return {(a.a + b.a) / 2, (a.b + b.b) / 2, (a.c + b.c) / 2};
}

int criticalPlaneBelow(double x)
{
    return static_cast<int>(std::floor(x - 0.5));
}

int criticalPlaneAbove(double x)
{
    return static_cast<int>(std::ceil(x - 0.5));
}

MdVec3 bisectToLimit(double y, double targetHue)
{
    MdVec3 segment[2];
    bisectToSegment(y, targetHue, segment);
    MdVec3 left = segment[0];
    double leftHue = hueOf(left);
    MdVec3 right = segment[1];
    for (int axis = 0; axis < 3; ++axis) {
        if (getAxis(left, axis) != getAxis(right, axis)) {
            int lPlane = -1;
            int rPlane = 255;
            if (getAxis(left, axis) < getAxis(right, axis)) {
                lPlane = criticalPlaneBelow(trueDelinearized(getAxis(left, axis)));
                rPlane = criticalPlaneAbove(trueDelinearized(getAxis(right, axis)));
            } else {
                lPlane = criticalPlaneAbove(trueDelinearized(getAxis(left, axis)));
                rPlane = criticalPlaneBelow(trueDelinearized(getAxis(right, axis)));
            }
            for (int i = 0; i < 8; ++i) {
                if (std::abs(rPlane - lPlane) <= 1) {
                    break;
                }
                const int mPlane = static_cast<int>(std::floor((lPlane + rPlane) / 2.0));
                const double midPlaneCoordinate = kCriticalPlanes[mPlane];
                const MdVec3 mid = setCoordinate(left, midPlaneCoordinate, right, axis);
                const double midHue = hueOf(mid);
                if (areInCyclicOrder(leftHue, targetHue, midHue)) {
                    right = mid;
                    rPlane = mPlane;
                } else {
                    left = mid;
                    leftHue = midHue;
                    lPlane = mPlane;
                }
            }
        }
    }
    return midpoint(left, right);
}

double inverseChromaticAdaptation(double adapted)
{
    const double adaptedAbs = std::fabs(adapted);
    const double base = std::fmax(0.0, 27.13 * adaptedAbs / (400.0 - adaptedAbs));
    return signum(adapted) * std::pow(base, 1.0 / 0.42);
}

Argb findResultByJ(double hueRadians, double chroma, double y)
{
    double j = std::sqrt(y) * 11.0;
    const ViewingConditions &vc = kDefaultViewingConditions;
    const double tInnerCoeff =
        1.0 / std::pow(1.64 - std::pow(0.29, vc.backgroundYToWhitePointY), 0.73);
    const double eHue = 0.25 * (std::cos(hueRadians + 2.0) + 3.8);
    const double p1 = eHue * (50000.0 / 13.0) * vc.nC * vc.ncb;
    const double hSin = std::sin(hueRadians);
    const double hCos = std::cos(hueRadians);
    for (int iterationRound = 0; iterationRound < 5; ++iterationRound) {
        const double jNormalized = j / 100.0;
        const double alpha = (chroma == 0.0 || j == 0.0) ? 0.0 : chroma / std::sqrt(jNormalized);
        const double t = std::pow(alpha * tInnerCoeff, 1.0 / 0.9);
        const double ac = vc.aw * std::pow(jNormalized, 1.0 / vc.c / vc.z);
        const double p2 = ac / vc.nbb;
        const double gamma = 23.0 * (p2 + 0.305) * t / (23.0 * p1 + 11.0 * t * hCos + 108.0 * t * hSin);
        const double a = gamma * hCos;
        const double b = gamma * hSin;
        const double rA = (460.0 * p2 + 451.0 * a + 288.0 * b) / 1403.0;
        const double gA = (460.0 * p2 - 891.0 * a - 261.0 * b) / 1403.0;
        const double bA = (460.0 * p2 - 220.0 * a - 6300.0 * b) / 1403.0;
        const MdVec3 scaled{inverseChromaticAdaptation(rA),
                            inverseChromaticAdaptation(gA),
                            inverseChromaticAdaptation(bA)};
        const MdVec3 linrgb = matrixMultiply(scaled, kLinrgbFromScaledDiscount);
        if (linrgb.a < 0 || linrgb.b < 0 || linrgb.c < 0) {
            return 0;
        }
        const double fnj =
            kYFromLinrgb[0] * linrgb.a + kYFromLinrgb[1] * linrgb.b + kYFromLinrgb[2] * linrgb.c;
        if (fnj <= 0) {
            return 0;
        }
        if (iterationRound == 4 || std::abs(fnj - y) < 0.002) {
            if (linrgb.a > 100.01 || linrgb.b > 100.01 || linrgb.c > 100.01) {
                return 0;
            }
            return MdColorMath::argbFromLinrgb(linrgb);
        }
        j = j - (fnj - y) * j / (2 * fnj);
    }
    return 0;
}

Argb solveToInt(double hueDegrees, double chroma, double lstar)
{
    if (chroma < 0.0001 || lstar < 0.0001 || lstar > 99.9999) {
        return MdColorMath::intFromLstar(lstar);
    }
    hueDegrees = MdColorMath::sanitizeDegreesDouble(hueDegrees);
    const double hueRadians = hueDegrees / 180.0 * kPi;
    const double y = MdColorMath::yFromLstar(lstar);
    const Argb exactAnswer = findResultByJ(hueRadians, chroma, y);
    if (exactAnswer != 0) {
        return exactAnswer;
    }
    const MdVec3 linrgb = bisectToLimit(y, hueRadians);
    return MdColorMath::argbFromLinrgb(linrgb);
}

// ---------------------------------------------------------------------------
// KeyColor (cpp/palettes/tones.cc), transcribed.
// ---------------------------------------------------------------------------

class KeyColor
{
public:
    KeyColor(double hue, double requestedChroma)
        : m_hue(hue)
        , m_requestedChroma(requestedChroma)
    {
    }

    MdHct create()
    {
        constexpr int pivotTone = 50;
        constexpr int toneStepSize = 1;
        constexpr double epsilon = 0.01;

        int lowerTone = 0;
        int upperTone = 100;
        while (lowerTone < upperTone) {
            const int midTone = (lowerTone + upperTone) / 2;
            const bool isAscending = maxChroma(midTone) < maxChroma(midTone + toneStepSize);
            const bool sufficientChroma = maxChroma(midTone) >= m_requestedChroma - epsilon;

            if (sufficientChroma) {
                if (std::abs(lowerTone - pivotTone) < std::abs(upperTone - pivotTone)) {
                    upperTone = midTone;
                } else {
                    if (lowerTone == midTone) {
                        return MdHct(m_hue, m_requestedChroma, lowerTone);
                    }
                    lowerTone = midTone;
                }
            } else {
                if (isAscending) {
                    lowerTone = midTone + toneStepSize;
                } else {
                    upperTone = midTone;
                }
            }
        }
        return MdHct(m_hue, m_requestedChroma, lowerTone);
    }

private:
    double maxChroma(double tone)
    {
        const auto it = m_chromaCache.find(tone);
        if (it != m_chromaCache.end()) {
            return it->second;
        }
        const double chroma = MdHct(m_hue, m_maxChromaValue, tone).chroma();
        m_chromaCache[tone] = chroma;
        return chroma;
    }

    const double m_maxChromaValue = 200.0;
    double m_hue;
    double m_requestedChroma;
    std::unordered_map<double, double> m_chromaCache;
};

} // namespace

// ---------------------------------------------------------------------------
// MdColorMath
// ---------------------------------------------------------------------------

Argb MdColorMath::argbFromRgb(int red, int green, int blue)
{
    return 0xFF000000u | (static_cast<Argb>(red & 0xff) << 16)
           | (static_cast<Argb>(green & 0xff) << 8) | static_cast<Argb>(blue & 0xff);
}

Argb MdColorMath::argbFromLinrgb(MdVec3 linrgb)
{
    const int r = delinearized(linrgb.a);
    const int g = delinearized(linrgb.b);
    const int b = delinearized(linrgb.c);
    return 0xFF000000u | (static_cast<Argb>(r & 0xff) << 16)
           | (static_cast<Argb>(g & 0xff) << 8) | static_cast<Argb>(b & 0xff);
}

int MdColorMath::redFromArgb(Argb argb)
{
    return static_cast<int>((argb & 0x00ff0000u) >> 16);
}

int MdColorMath::greenFromArgb(Argb argb)
{
    return static_cast<int>((argb & 0x0000ff00u) >> 8);
}

int MdColorMath::blueFromArgb(Argb argb)
{
    return static_cast<int>(argb & 0x000000ffu);
}

int MdColorMath::alphaFromArgb(Argb argb)
{
    return static_cast<int>((argb & 0xff000000u) >> 24);
}

bool MdColorMath::isOpaque(Argb argb)
{
    return alphaFromArgb(argb) == 255;
}

Argb MdColorMath::argbFromHex(const QString &hex)
{
    QString value = hex.trimmed();
    if (value.startsWith(QLatin1Char('#'))) {
        value = value.mid(1);
    }
    if (value.size() == 6) {
        value.prepend(QStringLiteral("ff"));
    }
    if (value.size() != 8) {
        return 0xFF000000u;
    }
    bool ok = false;
    const quint32 parsed = value.toUInt(&ok, 16);
    return ok ? static_cast<Argb>(parsed) : 0xFF000000u;
}

QString MdColorMath::hexFromArgb(Argb argb)
{
    return QString::number(argb, 16).rightJustified(8, QLatin1Char('0'));
}

QString MdColorMath::prettyHex(Argb argb)
{
    return QStringLiteral("#") + QString::number(argb & 0x00ffffffu, 16).rightJustified(6, QLatin1Char('0'));
}

double MdColorMath::linearized(int rgbComponent)
{
    const double normalized = rgbComponent / 255.0;
    if (normalized <= 0.040449936) {
        return normalized / 12.92 * 100.0;
    }
    return std::pow((normalized + 0.055) / 1.055, 2.4) * 100.0;
}

int MdColorMath::delinearized(double rgbComponent)
{
    const double normalized = rgbComponent / 100.0;
    double delinearizedValue = 0.0;
    if (normalized <= 0.0031308) {
        delinearizedValue = normalized * 12.92;
    } else {
        delinearizedValue = 1.055 * std::pow(normalized, 1.0 / 2.4) - 0.055;
    }
    return std::clamp(static_cast<int>(std::round(delinearizedValue * 255.0)), 0, 255);
}

double MdColorMath::lstarFromArgb(Argb argb)
{
    const double redL = linearized(redFromArgb(argb));
    const double greenL = linearized(greenFromArgb(argb));
    const double blueL = linearized(blueFromArgb(argb));
    const double y = 0.2126 * redL + 0.7152 * greenL + 0.0722 * blueL;
    return lstarFromY(y);
}

double MdColorMath::yFromLstar(double lstar)
{
    constexpr double ke = 8.0;
    if (lstar > ke) {
        const double cubeRoot = (lstar + 16.0) / 116.0;
        return cubeRoot * cubeRoot * cubeRoot * 100.0;
    }
    return lstar / (24389.0 / 27.0) * 100.0;
}

double MdColorMath::lstarFromY(double y)
{
    constexpr double e = 216.0 / 24389.0;
    const double yNormalized = y / 100.0;
    if (yNormalized <= e) {
        return (24389.0 / 27.0) * yNormalized;
    }
    return 116.0 * std::pow(yNormalized, 1.0 / 3.0) - 16.0;
}

Argb MdColorMath::intFromLstar(double lstar)
{
    const double y = yFromLstar(lstar);
    const int component = delinearized(y);
    return argbFromRgb(component, component, component);
}

double MdColorMath::sanitizeDegreesDouble(double degrees)
{
    if (degrees < 0.0) {
        return std::fmod(degrees, 360.0) + 360.0;
    }
    if (degrees >= 360.0) {
        return std::fmod(degrees, 360.0);
    }
    return degrees;
}

int MdColorMath::sanitizeDegreesInt(int degrees)
{
    if (degrees < 0) {
        return (degrees % 360) + 360;
    }
    if (degrees >= 360) {
        return degrees % 360;
    }
    return degrees;
}

double MdColorMath::diffDegrees(double a, double b)
{
    return 180.0 - std::abs(std::abs(a - b) - 180.0);
}

MdCam16 MdColorMath::camFromInt(Argb argb)
{
    const double redL = linearized(redFromArgb(argb));
    const double greenL = linearized(greenFromArgb(argb));
    const double blueL = linearized(blueFromArgb(argb));
    const double x = 0.41233895 * redL + 0.35762064 * greenL + 0.18051042 * blueL;
    const double y = 0.2126 * redL + 0.7152 * greenL + 0.0722 * blueL;
    const double z = 0.01932141 * redL + 0.11916382 * greenL + 0.95034478 * blueL;
    return camFromXyzAndViewingConditionsImpl(x, y, z, kDefaultViewingConditions);
}

MdCam16 MdColorMath::camFromXyz(double x, double y, double z)
{
    return camFromXyzAndViewingConditionsImpl(x, y, z, kDefaultViewingConditions);
}

double MdColorMath::camDistance(const MdCam16 &a, const MdCam16 &b)
{
    const double dJ = a.jstar - b.jstar;
    const double dA = a.astar - b.astar;
    const double dB = a.bstar - b.bstar;
    const double dEPrime = std::sqrt(dJ * dJ + dA * dA + dB * dB);
    return 1.41 * std::pow(dEPrime, 0.63);
}

MdCam16 MdColorMath::camFromJch(double j, double c, double h)
{
    const ViewingConditions &vc = kDefaultViewingConditions;
    const double q = (4.0 / vc.c) * std::sqrt(j / 100.0) * (vc.aw + 4.0) * vc.flRoot;
    const double m = c * vc.flRoot;
    const double alpha = c / std::sqrt(j / 100.0);
    const double s = 50.0 * std::sqrt((alpha * vc.c) / (vc.aw + 4.0));
    const double hueRadians = h * kPi / 180.0;
    const double jstar = (1.0 + 100.0 * 0.007) * j / (1.0 + 0.007 * j);
    const double mstar = 1.0 / 0.0228 * std::log(1.0 + 0.0228 * m);
    const double astar = mstar * std::cos(hueRadians);
    const double bstar = mstar * std::sin(hueRadians);
    return {h, c, j, q, m, s, jstar, astar, bstar};
}

MdCam16 MdColorMath::camFromUcs(double jstar, double astar, double bstar)
{
    const ViewingConditions &vc = kDefaultViewingConditions;
    const double a = astar;
    const double b = bstar;
    const double m = std::sqrt(a * a + b * b);
    const double m2 = (std::exp(m * 0.0228) - 1.0) / 0.0228;
    const double c = m2 / vc.flRoot;
    double h = std::atan2(b, a) * (180.0 / kPi);
    if (h < 0.0) {
        h += 360.0;
    }
    const double j = jstar / (1.0 - (jstar - 100.0) * 0.007);
    return camFromJch(j, c, h);
}

Argb MdColorMath::intFromCam(const MdCam16 &cam)
{
    const ViewingConditions &vc = kDefaultViewingConditions;
    const double alpha = (cam.chroma == 0.0 || cam.j == 0.0) ? 0.0 : cam.chroma / std::sqrt(cam.j / 100.0);
    const double t = std::pow(
        alpha / std::pow(1.64 - std::pow(0.29, vc.backgroundYToWhitePointY), 0.73), 1.0 / 0.9);
    const double hRad = cam.hue * kPi / 180.0;
    const double eHue = 0.25 * (std::cos(hRad + 2.0) + 3.8);
    const double ac = vc.aw * std::pow(cam.j / 100.0, 1.0 / vc.c / vc.z);
    const double p1 = eHue * (50000.0 / 13.0) * vc.nC * vc.ncb;
    const double p2 = ac / vc.nbb;
    const double hSin = std::sin(hRad);
    const double hCos = std::cos(hRad);
    const double gamma =
        23.0 * (p2 + 0.305) * t / (23.0 * p1 + 11.0 * t * hCos + 108.0 * t * hSin);
    const double a = gamma * hCos;
    const double b = gamma * hSin;
    const double rA = (460.0 * p2 + 451.0 * a + 288.0 * b) / 1403.0;
    const double gA = (460.0 * p2 - 891.0 * a - 261.0 * b) / 1403.0;
    const double bA = (460.0 * p2 - 220.0 * a - 6300.0 * b) / 1403.0;

    const double rCBase = std::fmax(0.0, (27.13 * std::fabs(rA)) / (400.0 - std::fabs(rA)));
    const double rC = signum(rA) * (100.0 / vc.fl) * std::pow(rCBase, 1.0 / 0.42);
    const double gCBase = std::fmax(0.0, (27.13 * std::fabs(gA)) / (400.0 - std::fabs(gA)));
    const double gC = signum(gA) * (100.0 / vc.fl) * std::pow(gCBase, 1.0 / 0.42);
    const double bCBase = std::fmax(0.0, (27.13 * std::fabs(bA)) / (400.0 - std::fabs(bA)));
    const double bC = signum(bA) * (100.0 / vc.fl) * std::pow(bCBase, 1.0 / 0.42);

    const double rX = rC / vc.rgbD[0];
    const double gX = gC / vc.rgbD[1];
    const double bX = bC / vc.rgbD[2];
    const double x = 1.86206786 * rX - 1.01125463 * gX + 0.14918677 * bX;
    const double y = 0.38752654 * rX + 0.62144744 * gX - 0.00897398 * bX;
    const double z = -0.01584150 * rX - 0.03412294 * gX + 1.04996444 * bX;

    const double rL = 3.2406 * x - 1.5372 * y - 0.4986 * z;
    const double gL = -0.9689 * x + 1.8758 * y + 0.0415 * z;
    const double bL = 0.0557 * x - 0.2040 * y + 1.0570 * z;

    return argbFromRgb(delinearized(rL), delinearized(gL), delinearized(bL));
}

Argb MdColorMath::intFromHcl(double hueDegrees, double chroma, double lstar)
{
    return solveToInt(hueDegrees, chroma, lstar);
}

double MdColorMath::contrastRatio(Argb a, Argb b)
{
    const double l1 = yFromLstar(lstarFromArgb(a));
    const double l2 = yFromLstar(lstarFromArgb(b));
    const double lighter = std::max(l1, l2);
    const double darker = std::min(l1, l2);
    return (lighter + 5.0) / (darker + 5.0);
}

Argb MdColorMath::onColorFor(Argb background, Argb lightCandidate, Argb darkCandidate)
{
    const double lightRatio = contrastRatio(background, lightCandidate);
    const double darkRatio = contrastRatio(background, darkCandidate);
    return lightRatio >= darkRatio ? lightCandidate : darkCandidate;
}

bool MdColorMath::tonePrefersLightForeground(double tone)
{
    // material-color-utilities DynamicColor.tonePrefersLightForeground().
    return tone < 49.5;
}

Argb MdColorMath::readableForegroundFor(Argb background)
{
    // Pure black and pure white, chosen by the same tone threshold MD3 applies
    // to its own surfaces. Deliberately not a token lookup.
    return tonePrefersLightForeground(lstarFromArgb(background)) ? 0xFFFFFFFFu : 0xFF000000u;
}

// ---------------------------------------------------------------------------
// MdHct
// ---------------------------------------------------------------------------

MdHct::MdHct()
    : m_argb(0xFF000000u)
{
}

MdHct::MdHct(double hue, double chroma, double tone)
{
    setInternalState(solveToInt(hue, chroma, tone));
}

MdHct::MdHct(Argb argb)
{
    setInternalState(argb);
}

void MdHct::setHue(double hue)
{
    setInternalState(solveToInt(hue, m_chroma, m_tone));
}

void MdHct::setChroma(double chroma)
{
    setInternalState(solveToInt(m_hue, chroma, m_tone));
}

void MdHct::setTone(double tone)
{
    setInternalState(solveToInt(m_hue, m_chroma, tone));
}

void MdHct::setInternalState(Argb argb)
{
    m_argb = argb;
    const MdCam16 cam = MdColorMath::camFromInt(argb);
    m_hue = cam.hue;
    m_chroma = cam.chroma;
    m_tone = MdColorMath::lstarFromArgb(argb);
}

// ---------------------------------------------------------------------------
// MdTonalPalette
// ---------------------------------------------------------------------------

MdTonalPalette::MdTonalPalette() = default;

MdTonalPalette::MdTonalPalette(Argb argb)
{
    const MdCam16 cam = MdColorMath::camFromInt(argb);
    m_hue = cam.hue;
    m_chroma = cam.chroma;
    m_keyColor = KeyColor(cam.hue, cam.chroma).create();
}

MdTonalPalette::MdTonalPalette(double hue, double chroma)
    : m_hue(hue)
    , m_chroma(chroma)
    , m_keyColor(KeyColor(hue, chroma).create())
{
}

MdTonalPalette::MdTonalPalette(const MdHct &hct)
    : m_hue(hct.hue())
    , m_chroma(hct.chroma())
    , m_keyColor(hct.hue(), hct.chroma(), hct.tone())
{
}

MdLab MdTonalPalette::labFromArgb(Argb argb)
{
    // lab.cc LabFromInt.
    const double redL = MdColorMath::linearized(MdColorMath::redFromArgb(argb));
    const double greenL = MdColorMath::linearized(MdColorMath::greenFromArgb(argb));
    const double blueL = MdColorMath::linearized(MdColorMath::blueFromArgb(argb));
    const double x = 0.41233895 * redL + 0.35762064 * greenL + 0.18051042 * blueL;
    const double y = 0.2126 * redL + 0.7152 * greenL + 0.0722 * blueL;
    const double z = 0.01932141 * redL + 0.11916382 * greenL + 0.95034478 * blueL;

    constexpr double e = 216.0 / 24389.0;
    constexpr double kappa = 24389.0 / 27.0;

    const double yNormalized = y / 100.0;
    const double fy = yNormalized > e ? std::cbrt(yNormalized) : (kappa * yNormalized + 16.0) / 116.0;
    const double xNormalized = x / 95.047;
    const double fx = xNormalized > e ? std::cbrt(xNormalized) : (kappa * xNormalized + 16.0) / 116.0;
    const double zNormalized = z / 108.883;
    const double fz = zNormalized > e ? std::cbrt(zNormalized) : (kappa * zNormalized + 16.0) / 116.0;

    return {116.0 * fy - 16.0, 500.0 * (fx - fy), 200.0 * (fy - fz)};
}

Argb MdTonalPalette::tone(double tone) const
{
    return MdColorMath::intFromHcl(m_hue, m_chroma, tone);
}

// ---------------------------------------------------------------------------
// MdCorePalette
// ---------------------------------------------------------------------------

MdCorePalette::MdCorePalette(Argb seed)
{
    // CorePalette.of(argb) — cpp/palettes/core.h + tones.cc semantics.
    const MdHct hct(seed);
    const double hue = hct.hue();
    const double chroma = hct.chroma();

    m_primary = MdTonalPalette(hue, chroma);
    m_secondary = MdTonalPalette(hue, chroma / 3.0);
    m_tertiary = MdTonalPalette(MdColorMath::sanitizeDegreesDouble(hue + 60.0), chroma / 2.0);
    m_neutral = MdTonalPalette(hue, std::min(chroma / 12.0, 4.0));
    m_neutralVariant = MdTonalPalette(hue, std::min(chroma / 6.0, 8.0));
    // MD3 always derives error from a fixed hue/chroma pair.
    m_error = MdTonalPalette(25.0, 84.0);
}

} // namespace md
