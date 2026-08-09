# arduino monochromator controller
This repository is for an Arduino-based automation controller for an Optical Building Blocks (OBB) monochromator in the Greytak lab at the University of South Carolina. This monochromator was originally a manually-operated model and has been enhanced with a NEMA-17 bipolar stepper motor, driven by a DRV8825 stepper motor controller mounted on an Arduino shield. A single limit switch completes the electronic design. 

This project is an implementation of the "Arduino Basic Laboratory Instrument" described in our `arduino-BLI` repository, and uses an Arduino Uno R3 microcontroller with peripheral button/LED and 16x2 LCD display boards.

Developed by the Greytak lab at the University of South Carolina. Copyright Andrew B. Greytak 2026.

For more information about our laboratory see the Greytak Chemisty Lab's [GitHub Pages site](https://greytak-chemistry-lab.github.io), or our group website.

## Purpose and overview of project

These notes are incomplete and should be improved over time.

Configuration information:
- Some instrument-specific parameters including micro-stepping configuration for the stepper motor drives (i.e., whether it will simulate a larger number of steps per revolution), steps-to-wavelength calibration, and home position are specified in the Arduino sketch.
- On startup, the program will ask if homing is necessary (it will be if the instrument was powered off in position other than home).

After startup, the program operates in two modes, selected by the mode toggle switch.
- In **manual** mode, the user can specify a desired target wavelength and a speed. The stepper motor will then move the grating to the desired position.
- In **auto** mode, the user will specify parameters for the starting wavelength, ending wavelength, speed, and number of replicates for a desired wavelength scan.
	- After setting these parameters, `Start/Next` will begin the scan. `Start/Next` normally uses a pushbutton input, but could be triggered by an active-low external signal. The LED is activated at the start of each sweep, and could be used to trigger an external recording device.
	- Pressing `Stop/Reset` will pause the scan. Holding it down will enable reset. See program comments for more info on error handling, for which this project is a good example.
	- At the conclusion of the desired replicates, the program will await the next `Start/Next` signal to repeat. Toggling to manual mode can enable re-entry of auto scan parameters.
	

## Repository organization (replicated in each variation-version):

- **`layouts`**: folder with description of hardware
    - Should include parts list(s) and diagrams
	- Could have subfolders for different sub-assemblies, or for minor variations in design or accessories
- **`programs`**: Software
	- Can have subfolders for different programs that can run on the same hardware
	- Remember that Arduino sketches need to live in a subfolder where the folder name and `.ino` file name are the same
- **`calibration results`**: Instrument specific data
	- used to select parameters needed for programs


## Our approach to git repositories: variations and versions

The current Greytak lab approach is that projects may proceed through "variations" and "versions". These will refer to subfolders that each include a replicate of the entire project as described above. A user will only be using a single variation/version at a time. All variations and versions will live in the same ("main") branch of the git repo. This simplifies administration by not requiring all lab members to understand the details of git branching/merging/pull requests etc. The maintainer of the repo may still choose to use branches on a short-term basis for some kind of experiment or bug fix. **Our use of these is a bit different for hardware vs code-only projects.**

Each repo will have one or more maintainers who are able to commit changes. The number of maintainers will be small -- usually Greytak and one student -- for continuity of style within the project. Other fixes or additions can be made by first demoing it in your own clone, and then asking the maintainer to copy it into the shared project.

"Variations" will refer to some major stage of development or intended features for a project. For hardware projects, this usually means different core hardware, not different software or peripherals that can be used with the same core device (those can just be added as subfolders in `layouts` or `programs`). If you do want to make some significant changes that may break compatibility with older hardware, it might be time to make a new "variation". Variations will have a name to identify them -- but this can be a codename and need not specify what the variation is for, since that could be hard to describe and could evolve over time. For most projects, we will start with a special "variation": `og` (original / original gangster), representing the project as it was when it was set up (copied over from our Subversion repos, usually). We may also have a variation representing the latest stage of development of the project. If you are about to add a major new capability, or some big algorithm or data storage change that could affect compatibility, you will want to copy the latest variation to a new "variation" folder. You may later decide to copy successful developments back into the previous variation, but your new variation could also remove features and go in a different direction. Be sure to talk to the maintainer and your lab-mates. For some repos, "current" may exist as a variation, with previous variations (if any other than "og") carved off later.

"Versions" will refer to incremental development of a variation. Versions have a number. We will use them like "tags" in Subversion. Typically, the "current" version is just the folder with the variation name only. When we are about to implement an incremental change that could affect compatibility, or if we need to capture the project just as it was at the time of some publication, **and for any situation where a program is to be used on laboratory hardware**, we will copy the "current" variation folder to a folder with the same name, but with the version number appended. Version numbers will typically be 0.1, 0.2 etc until the variation is reasonably useful, and then 1.0, 1.1 etc. Bug fixes and improved commenting can be applied (manually) to numbered "versions", but no changes should be made in them that will break compatibility. Usually, it will be preferable to do such things in the current version, rather than in a previous numbered one. **Please note that, after you copy the project into a numbered version folder and before you load it onto your hardware device**, you should change software in that folder to display the software version number on the device on startup, where that is possible.

Folders will be named (within the repo) as: "variation-version" or just "variation" for the current version of each variation.

Presently, only the `og` variation exists for this `arduino-bli-mono-controller` project, and it is designed to operate the mono with an infrared grating. Calibration information in the `og` variation folders applies to this implementation.

## Attribution and license:
> This is a project of the Greytak Chemistry Lab – `github.com/greytak-chemistry-lab/arduino-bli-mono-controller`
>
> Copyright Andrew B Greytak with applicable rights reserved by the University of South Carolina. Email: `greytak@sc.edu`.
> - Code shared via GNU Public License (GPL v3)
> - Layouts / Resources shared via CC-BY-SA-4.0
>
> See LICENSE file for details
