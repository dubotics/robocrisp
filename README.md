# RoboCRISP: A Clean Robotics Interface System for (real) People

RoboCRISP is a communication framework for real-time teleoperated robotics applications,
intended for use in an undergraduate-level college setting.  The CRISP architecture is designed
to provide an intuitive, "action-reaction"-type approach to the interfaces between robotics
platforms and their operator interfaces.

RoboCRISP was written for a student robotics organization at a large university, in which member
turnover _could_ make it difficult to maintain working knowlege of the organization's robotics
systems interfaces.  As such, the framework is designed with the goal of having some level of
self-documentation ability in deployed systems: given the basic tools to communicate with a
device using a CRISP communication protocol, it should be possible to fully control that device
_without_ access to the onboard code.

[And really?  Go ahead and make up your own backronym.  We won't mind.]


## Features/Disfeatures

RoboCRISP provides

  * an abstract, high-level, and *self-documenting* communication scheme that's purposely
    generic and independent of hardware interfaces;

  * easy-to-use, data-centric I²C utilities for a platform's internal (low-level)
    communications, compatible with both AVR-based microcontrollers and Linux-based
    microcomputers; and

  * a modular event-based approach to the control-to-communication pipeline for the operator end
    of the system.


RoboCRISP does **not** provide:

  * an end-to-end toolchain and workflow for your robotics project.  You probably already have
    your own workflow and toolchain, and RoboCRISP was *not* created to disrupt or
    change those.

  * a complete, ready-made "solution" for ...well, anything.  RoboCRISP provides the generic
	components for _networking_ the processors of an easily-understandable control system, but
	the glue required for low-level hardware control is outside its scope.


## Documentation

Documentation is [or will be, if it's the last thing I do...] provided in Markdown format inside
the `doc/` folder of the repository or project root.


## Contributors

RoboCRISP is maintained by the DUBotics robotics club at the University of Washington.
