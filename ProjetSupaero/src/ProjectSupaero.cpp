#include <iostream>
#include <fstream>
#include <time.h>

#include <acado_toolkit.hpp>
#include <acado_optimal_control.hpp>
#include <acado_gnuplot.hpp>

using std::cout; using std::endl;


int main()
{
    USING_NAMESPACE_ACADO


    // INTRODUCE THE VARIABLES:
    // -------------------------
    DifferentialState x,y,z, vx,vy,vz, phi,theta,psi, p,q,r, u1,u2,u3,u4;
    // x, y, z : position
    // vx, vy, vz : linear velocity
    // phi, theta, psi : orientation (Yaw-Pitch-Roll = Euler(3,2,1))
    // p, q, r : angular velocity
    // u1, u2, u3, u4 : velocity of the propellers
    Control vu1,vu2,vu3,vu4;
    // vu1, vu2, vu3, vu4 : derivative of u1, u2, u3, u4

    // Quad constants
    const double c  = 0.00001;
    const double Cf = 0.00065;
    const double d  = 0.250;
    const double Jx = 0.018;
    const double Jy = 0.018;
    const double Jz = 0.026;
//    const double Im = 0.0001;
    const double m  = 0.9;
    const double g  = 9.81;
//    const double Cx = 0.1;

    // DEFINE A DIFFERENTIAL EQUATION:
    // -------------------------------
    DifferentialEquation f;

    f << dot(x) == vx;
    f << dot(y) == vy;
    f << dot(z) == vz;
    f << dot(vx) == Cf*(u1*u1+u2*u2+u3*u3+u4*u4)*sin(theta)/m;
    f << dot(vy) == -Cf*(u1*u1+u2*u2+u3*u3+u4*u4)*sin(psi)*cos(theta)/m;
    f << dot(vz) == Cf*(u1*u1+u2*u2+u3*u3+u4*u4)*cos(psi)*cos(theta)/m - g;
    f << dot(phi) == -cos(phi)*tan(theta)*p+sin(phi)*tan(theta)*q+r;
    f << dot(theta) == sin(phi)*p+cos(phi)*q;
    f << dot(psi) == cos(phi)/cos(theta)*p-sin(phi)/cos(theta)*q;
    f << dot(p) == (d*Cf*(u1*u1-u2*u2)+(Jy-Jz)*q*r)/Jx;
    f << dot(q) == (d*Cf*(u4*u4-u3*u3)+(Jz-Jx)*p*r)/Jy;
    f << dot(r) == (c*(u1*u1+u2*u2-u3*u3-u4*u4)+(Jx-Jy)*p*q)/Jz;
    f << dot(u1) == vu1;
    f << dot(u2) == vu2;
    f << dot(u3) == vu3;
    f << dot(u4) == vu4;


    // SET UP THE (SIMULATED) PROCESS:
    // -----------------------------------
    OutputFcn identity;
    DynamicSystem dynamicSystem(f,OutputFcn{});
    Process process(dynamicSystem,INT_RK45);


    // DEFINE LEAST SQUARE FUNCTION:
    // -----------------------------
    Function h;
    h << x << y << z;
    h << vu1 << vu2 << vu3 << vu4;
    h << p << q << r;

    // LSQ coefficient matrix
    DMatrix Q(10,10);
    Q(0,0) = Q(1,1) = Q(2,2) = 1e-1;
    Q(3,3) = Q(4,4) = Q(5,5) = Q(6,6) = 1e-9;
    Q(7,7) = Q(8,8) = Q(9,9) = 1e-3;

    // Reference
    double ref[10] = {0., 0., 20., 0., 0., 0., 0., 0., 0., 0.};
    DVector refVec(10, ref);
    // Reference 2, to see if we can change reference once the solver is started
    double ref2[10] = {0., 0., 0., 0., 0., 0., 0., 0., 0., 0.};
    DVector refVec2(10, ref2);


    // DEFINE AN OPTIMAL CONTROL PROBLEM:
    // ----------------------------------
    OCP ocp(0., 1., 20);

    ocp.minimizeLSQ(Q, h, refVec);

    ocp.subjectTo(f);    // Constraints on the velocity of each propeller
    ocp.subjectTo(16 <= u1 <= 95);
    ocp.subjectTo(16 <= u2 <= 95);
    ocp.subjectTo(16 <= u3 <= 95);
    ocp.subjectTo(16 <= u4 <= 95);

    // Command constraints
    // Constraints on the acceleration of each propeller
    ocp.subjectTo(-100 <= vu1 <= 100);
    ocp.subjectTo(-100 <= vu2 <= 100);
    ocp.subjectTo(-100 <= vu3 <= 100);
    ocp.subjectTo(-100 <= vu4 <= 100);

    // Constraint to avoid singularity
    ocp.subjectTo(-1. <= theta <= 1.);

    // Example of Eliptic obstacle constraints (here, cylinders with eliptic basis)
//    _ocp.subjectTo(16 <= ((x+3)*(x+3)+2*(z-5)*(z-5)));
//    _ocp.subjectTo(16 <= ((x-3)*(x-3)+2*(z-9)*(z-9)));
//    _ocp.subjectTo(16 <= ((x+3)*(x+3)+2*(z-15)*(z-15)));


    // SET UP THE MPC CONTROLLER:
    // ------------------------------
    RealTimeAlgorithm alg(ocp,0.025);
    alg.set(INTEGRATOR_TYPE, INT_RK78);
    alg.set(MAX_NUM_ITERATIONS,1);
    alg.set(PRINT_COPYRIGHT, false);

    Controller controller(alg);

    // SETTING UP THE SIMULATION ENVIRONMENT,  RUN THE EXAMPLE...
    // ----------------------------------------------------------
    DVector X(16), U(4);
    X.setZero(16);
    U.setZero(4);
    controller.init(0., X);
    process.init(0., X, U);

    VariablesGrid Y, graph;
    Y.setZero();

//    VariablesGrid newRef(refVec2, Grid{0., 5., 2});

    double t=0;
    for (int i=0; i<300; i++, t+=.025)
    {
//        if (i==100)
//        {
//            alg.setReference(newRef);
//        }
        process.getY(Y);
        X = Y.getLastVector();
        controller.step(t, X);
        controller.getU(U);
        process.step(t,t+0.025,U);

        graph.addVector(X,t);
    }

    GnuplotWindow window;
    window.addSubplot(graph(0), "x");
    window.addSubplot(graph(1), "y");
    window.addSubplot(graph(2), "z");
    window.plot();

    return 0;
}
