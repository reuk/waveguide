---
layout: page
title: Conclusion
navigation_weight: 24 
---

---
reference-section-title: References
...

# Conclusion {.major}

## Analysis

### Testing Procedure

Due to time constraints, the tests of Wayverb, presented in the
[Evaluation]({{site.baseurl}}{% link evaluation.md %}), are not exhaustive.
Only one of the research goals, namely plausibility, is examined. However, the
examination does not consider all aspects of this goal.  Ideally a more
thorough testing procedure, such as that used in [@southern_room_2013], would
have been followed.  That paper presents test results for the boundary
performance, modal content, diffraction, and reverb times of acoustic models
including FDTD, FEM, image-source model and *beam-tree acoustic radiance
transfer*. A wide range of results for each test is shown: the modal response
of the FDTD model is compared to that of the image source model for six
different reflection coefficient values, and the reverb time predicted by each
model is shown for a range of ten different reflection coefficient values.
This level of detail gives a clear view of the models' performance. Due to the
narrow range of parameters tested with Wayverb, it is possible that the
implementation may exhibit issues for some combination of parameters not
presented here. It would be desirable to test a wider range of parameters to
find out whether Wayverb performs appropriately under all circumstances.

The summaries of the chapters concerning microphone and boundary models both
describe how the components in question work in isolation. However, the models
have not been tested in the context of a long-running simulation.  In
particular, it has not been shown how either model affects the overall
frequency response and reverb time in the waveguide. The lack of tests in these
areas may hide latent issues in Wayverb's implementation.

All tests presented throughout this paper use approximations to find the
desired or target results.  For instance, the test of the waveguide modal
response uses the image-source method as a target, and the reverb time tests in
the [Evaluation]({{site.baseurl}}{% link evaluation.md %}) compare the
simulation results against the Sabine reverb time. These approximations have
shortcomings, as discussed in the relevant sections. It would be more useful to
directly compare the results obtained against experimentally-obtained IRs of
real locations, which would have no such shortcomings. If the simulated results
were shown to be within a "just noticeable difference" of the real results,
this would be strong evidence of the simulation's suitability for prediction of
architectural acoustics. These tests would be time consuming, not just because
impulse responses would have to be recorded in a physical location, but also
because that same location would need to be reconstructed virtually, using the
same dimensions and surface characteristics. In particular, measuring the
absorption and scattering coefficients for each material in the room would
require access to a specialised reverberation chamber, or these coefficients
would have to be approximated using published values.

As an extension to modelling physical locations, it may be instructive to
conduct listening tests. Such tests might aim to test whether listeners can
tell apart simulation results and experimentally-obtained IRs. If the
simulation results were shown to be indistinguishable from real IRs, this might
indicate that Wayverb is suitable for realistic sound-design applications.  It
might also be useful to conduct listening tests to gauge the (subjective)
perceived sound quality of simulation outputs. A simulation program that sounds
realistic but "bad" is not useful when producing music that is designed to be
enjoyed. If it could be shown that listeners perceived the sound quality to be
at least as good as real IRs, this would make a good case for Wayverb's utility
in a sound designer's toolbox.

Wayverb's results could be compared to the outputs of other similar modelling
programs, both through acoustic parameters like T30, and using listening tests.
As noted in the [Context]({{site.baseurl}}{% link context.md %}) chapter,
Wayverb is, at time of writing, the only publicly-available acoustics-modelling
tool that uses both geometric and waveguide models. Without comparing against
other tools, it is difficult to say whether this hybrid approach yields
tangible sound quality gains over a more traditional single-model approach.

The goals of efficiency and usability were not tested. In terms of efficiency,
it would be useful to show how simulation time varies with parameters such as
room size, average surface absorption, and model crossover frequency. The
simulation speed could also be compared to competing software packages.
Usability is more difficult to test, but a sensible starting point would be to
simply observe some creative professionals using the software (in order to find
common points of confusion or frustration) and to ask for feedback.

As it stands, the tests presented in the [Evaluation]({{site.baseurl}}{% link
evaluation.md %}) do not have sufficient scope in order to make claims about
Wayverb's overall "usefulness" as a software tool.  Further tests which
would help to gauge Wayverb subjectively, and in the context of competing
packages, have been suggested.

### Evaluation of Project Goals

#### Plausibility

All models presented in the [Evaluation]({{site.baseurl}}{% link evaluation.md
%}) perform as expected with regards to changes in room size and shape,
material coefficients, source and receiver positions, and receiver microphone
type. Reverb features such as distinct late echoes can be generated, and stereo
effects, relying on both interaural time- and level-difference, can be created.
The simulation responds appropriately to changes in parameters. For example,
increasing the absorption coefficients of surfaces leads to a decrease in
overall reverb time, and increasing the volume of the simulated room increases
the reverb time. In these respects, the Wayverb program achieves the goal of
physical plausibility.

Although parameter changes have the expected effects, the simulation is not
entirely successful.  The main drawback of the presented implementation,
evident in several of the tests, is that the geometric and wave-based models
have distinct behaviours. In the room-size and material tests, there are
markedly different reverb times between the ray-tracer and waveguide outputs;
and in the obstruction test, the waveguide exhibits diffraction behaviour which
is not mirrored in the geometric output. These differences lead to obvious
discontinuities in the frequency response, which persist despite calibrating
the models to produce the same sound level at the same distance, and
implementing matching boundary models in all models.

Some differences are to be expected. The primary reason for implementing
multiple simulation methods is the relative accuracy of the different methods.
Geometric algorithms are known to be inaccurate at low frequencies, a
shortcoming that the waveguide was designed to overcome.  However, the
magnitude of the difference in reverb time between the waveguide and ray-tracer
outputs (evident in +@fig:room_size_rt30) suggests that there are errors or
false assumptions in the implementation.  Even though the different components
of the waveguide (such as the boundary model, the source injection method, and
the microphone model) appear to work in isolation, the interactions between
these components are complex. Further testing of the waveguide as a whole, with
all these components enabled, is required in order to find the cause of its
lower-than-expected reverb times.

Even if further testing were somehow to reveal that the implementation is
correct, the current simulation results are useless if there is an obvious
disconnect between high- and low-frequency outputs. For applications like music
and sound design, where it is important that the generated responses are
believable, the current algorithm is unusable. Even if the low-frequency
response by itself accurately represents the modelled space, responses will
sound *more* artificial if there is a rapid change in the frequency response
when combined with the geometric output.  Indeed, it is preferable that the
frequency response not contain obvious discontinuities, even if this
necessitates a reduction in overall accuracy.

Practical solutions to this problem are unclear. Ideally, the entire simulation
would be run with the waveguide method, but this is impractical for all but the
smallest simulations. Another option is to reduce the audible impact of the
crossover between the waveguide and geometric outputs, simply by increasing its
width.  This has the dual drawbacks of decreasing the low-frequency accuracy,
while also requiring a higher waveguide sample-rate, which is computationally
expensive.  Alternatively, the geometric algorithms may be extended, to
account for effects such as diffraction, with the goal of minimising the
differences between the wave and ray-based methods. This solution would
maintain (or even improve) the accuracy of the simulation, but would again
increase its cost.  Finally, the goal of low-frequency accuracy could be
abandoned, and the entire spectrum modelled using geometric methods. However,
this would prevent important characteristics such as modal behaviour from
being recorded.

#### Efficiency

The efficiency goal put forward in the project aims stated that simulations
should, in general, take less than ten minutes to run. This held true for the
test cases presented in the [Evaluation]({{site.baseurl}}{% link evaluation.md
%}). However, the render time is still long enough to be distracting, and the
user experience could be greatly improved by decreasing its duration.  The
reason for the long render times is simple: the majority of the development
time on Wayverb was spent trying to ensure that each model component functioned
correctly. By the time the components were "complete" there was little time
left for optimising.

The length of the render time may be especially problematic in creative
contexts.  When producing music, it is important to be able to audition and
tweak reverbs in order to produce the most appropriate effect.  With long
rendering times, this auditioning process becomes protracted, which impacts the
productivity of the musician. In addition, if the program uses the majority of
the machine's resources while it is running, then this prevents the user from
running it in the background and continuing with other tasks in the meantime.
Usability tests would be required to find how problematic the render times are
in practice.

The simulation methods have been implemented to minimise average-case
computational complexity wherever possible, but both the ray-tracing and
waveguide processes are limited to a complexity of $O(n)$ where n refers to the
number of rays or nodes required in the simulation.  Any further performance
improvements may only be gained by improving the per-ray or per-node processing
speed. This is certainly possible, but would yield relatively small
improvements to the simulation speed. It may be equally valid to simply wait
for hardware with increased parallelism support: a machine with twice as many
graphics cores will run the program twice as fast. Such machines are likely
to be commonplace in two or three years. Therefore, a better use of time
would be to spend those two years focusing on the algorithm's functional
problems rather than optimisation. Although it is disappointing to fall back
on the argument that hardware improvements will obviate the need for software
efficiency improvements, note that Wayverb is uniquely well-placed to benefit
from hardware improvements going forward.  Newer computing platforms are
generally more powerful due to an increase in the number of processor cores
rather than an increase in overall clock speed, and Wayverb by design can
scale across all available GPU cores.

#### Accessibility

Considerations regarding the accessibility of the program were limited to the
design of the graphical interface. The target demographic of creative users was
considered in several ways.

The workflow is designed to be intuitive. When the app is opened, it asks for a
CAD file to load. Once a file has been selected, a list of settings will open
on the left side of the window, along with a graphical view of the 3D space.
The user can simply work down the list of settings, starting with the most
important parameters: source and receiver placement, and surface settings.  At
the bottom of the list is a large "render" button, which will open an exporter
dialog box when clicked. There are no unlabelled icons or options hidden in
menus; the entire simulation is controlled from a single window. This makes it
simple to get started with the app.

The controls themselves are simple and self-explanatory. The number of rays is
controlled by a single "quality" parameter, as opposed to allowing the user to
specify an exact number of rays. The allows the user to think in terms of the
output quality. If the ray number was directly exposed as a control, it may not
be clear how this parameter would affect the output. Similarly, the surface
settings dialog box is set out like a multiband EQ (with which users will be
familiar), which provides hints as to the effect of the absorption in each
band. Presets are also provided to give users a starting point when designing
their own materials.

The UI of the program was not only developed with users in mind. In the early
stages of the project, the 3D display of the room was used for debugging
aspects of the simulation algorithms, such as ray scattering, boundary node
placement, and waveguide node pressures over time. It was later extended to
allow positioning the sources and receivers through mouse interaction, and all
of the debugging options were removed.  Now, other than displaying source and
receiver positions, and the different surface regions, this display is of
limited use. The focus on debugging meant that too little time was spent
polishing the end-user experience in this part of the interface. In particular,
the material and source/receiver controls might be considerably more intuitive
if they could be accessed from the 3D view. For example, specific surfaces
could be modified by right-clicking them on the 3D model. Also, a receiver's
direction and polar pattern could be adjusted from controls which appeared
upon hovering. This would provide a more unified experience than the current
"menus and dialogs"-based approach.

As mentioned above, user tests would be required in order to evaluate the
accessibility of the current interface. Although the simulation behaves
plausibly in many respects, this may not translate into an intuitive control
scheme.  It is unknown whether a user with limited acoustics experience would
be able to create a reverb with the characteristics they desire. A potential
issue for such users is that familiar controls from parametric reverbs, such as
reverb time and EQ, are not directly available in Wayverb. This is not only
disorienting, it also necessitates a time-consuming workflow for some common
tasks. If the user desires a longer reverb time, they cannot simply move a
slider: they must open the 3D model in a CAD program, adjust its scale, and
then export it and reconfigure it in Wayverb.  Similarly, if a particular "EQ"
is desired, there is no way to adjust the frequency-dependent absorption of all
surfaces simultaneously. If the user is trying to simulate the sound of a
particular space this is not a problem.  However, if the user is trying to work
backwards, tweaking a space to produce a particular sound, Wayverb's controls
are unquestionably awkward.

A final usability issue is the dependency upon 3D models. Wayverb requires that
3D model files are solid and watertight, which in practice means that models
must be specially constructed, or validated before use. It is unreasonable to
expect musicians and sound designers to be proficient with professional CAD
software, so in order to be useful, a library of suitable 3D models should be
supplied alongside an installation of Wayverb.  In the [Context]({{
site.baseurl }}{% link context.md %}) chapter, the dependency upon libraries
of IRs was described as a drawback of convolution reverb tools, as it is
impossible for a library to cover all possible usage scenarios. However, it
appears that Wayverb relies upon libraries of 3D models. A possible solution
to this problem would be to integrate a custom 3D modelling tool into
Wayverb. This tool would only allow the creation of valid watertight spaces,
and could be much simpler and more minimal than professional CAD tools. Then,
users could quickly create spaces directly from the software, without having
to worry about whether or not the model is valid for simulation.

### Future Work

Shortcomings of the testing procedure have already been discussed, and it
should go without saying that future work on the project should begin by
testing existing features, rather than implementing new ones.  However, there
are many possibilities for features which might be considered for inclusion in
a future version of Wayverb.  These features can broadly be separated into two
categories: extensions to the simulation algorithm, and usability improvements
to the program interface.

#### Algorithm

As noted in the [Digital Waveguide Mesh]({{ site.baseurl }}{% link waveguide.md
%}) section, the input signal used to excite the waveguide is not optimal. Its
frequency response extends up to the Nyquist frequency, which means that the
mesh has energy above the desired output frequency. As shown in the [Boundary
Modelling]({{ site.baseurl }}{% link boundary.md %}) section, the performance
of the boundary filters is often erratic above 0.15 of the mesh sampling rate,
sometimes increasing rather than reducing gain of incident signals. In
combination, the broadband input signal sometimes causes the reflection filters
to repeatedly amplify high-frequency content in the mesh. This is not audible
in the final results, as the high frequency content is filtered out. However,
it still leads to loss of precision, and sometimes numeric overflow. This might
be solved by high-pass filtering the input signal, and then deconvolving the
mesh output.  However, it is not clear how such a process would interact with
the microphone simulation.  For example, it would probably be necessary to
record and deconvolve the signals at all nodes surrounding the output node in
order to produce a correct intensity vector. This would require further
development and testing, for which there was insufficient time during the
Wayverb project.

A similar drawback is to do with the low-frequency response of the mesh. Most
input signals cause an increasing DC offset when the mesh uses a "soft" input
node. To solve this, Wayverb's mesh is excited using a "hard" node, which
introduces low-frequency oscillations and reflection artefacts. An important
area for future research is the development of an input signal which can be
used in conjunction with a soft source, which does not cause any DC offset.

Wayverb's waveguide topology is currently rectilinear, which is suboptimal both
in terms of efficiency and accuracy. This topology was chosen because it was
the only one which supported the LRS boundary model. The tetrahedral topology
is faster and requires less memory. Therefore, future work may seek to
formulate the LRS boundary equations such that they can be used with a
tetrahedral mesh.

An important feature which is not implemented in Wayverb is the modelling of
directional sources.  Currently, all modelled sources emit a single spherical
wave-front, which has equal energy in all directions. Real-world sources such
as musical instruments and the human voice are directional. The ability to
model directional sources would allow musicians and sound designers to create
much more realistic and immersive acoustic simulations.

As well as directional sources, it might be useful to make the implementation
of directional receivers more generic. Specifically, an ambisonic receiver
would be useful, so that simulation results could be exported for directional
processing in dedicated software. This could be achieved without modifying the
geometric microphone model, in which coincident capsules are well-justified and
lead to performance improvements (the simulation is run once for the entire
capsule group, instead of once per individual capsule). However, the approach
is not strictly justified in combination with the current waveguide microphone
model [@hacihabiboglu_simulation_2010]. Ambisonic output would therefore
require further research into waveguide microphone modelling, in order to find
a model which better supports coincident capsules.

#### User Interface

The user interface is less fully-featured than the interfaces of similar
simulation programs (as listed in the [Context]({{ site.baseurl }}{% link
context.md %}) section, +@tbl:software).  The reason for this is simple: the
entire application was developed by a single developer, over sixteen months,
whereas all the software in +@tbl:software was either developed by several
collaborators, or was constructed over several years. To ensure that Wayverb
would reach a usable state, its scope had to be limited.  In its first release,
the application is only capable of loading, saving, configuring, and running
simulations. Examples of features generally found in acoustic simulation apps
but missing in Wayverb follow.

Originally, the app was designed to include built-in convolution support, so
that generated impulse responses could be previewed without leaving the
application. This feature would greatly improve the usability of the program.
However, it would not contribute to the main goal of the program, which is the
accurate and fast acoustic simulation of virtual environments.  Convolution
reverb tools already exist, and many users will have their own favourite
programs and plug-ins, like Altiverb [@altiverb] and Reverberate 2
[@reverberate2], for this purpose. The time that would have been spent
replicating this functionality was better spent working on the unique and novel
features of the program.

Similarly, the ability to edit the virtual spaces themselves from within the
app was not implemented. Writing an intuitive editor for 3D objects would be a
large undertaking, even for a team of developers.  Instead, the ability to load
a variety of 3D file formats was included, and users are advised to use
dedicated software such as Blender or Sketchup to create their simulated
spaces.

Some further usability features which are missing, which would ideally be
included in a future release, include:

- **Undo and redo**: If the user accidentally moves a source or receiver, or
  makes some other unwanted change, they must revert the change manually.  There
  is no way of automatically reverting to the previous program state.
- **Load and save of capsule and material presets**: If the model contains
  several surfaces with different materials, and the user wants to apply a
  specific set of coefficients to all surfaces, each material must be configured
  by hand. There is no way to copy coefficients between surfaces, or to save and
  load materials from disk. Similarly, there is no way to save and load complex
  receiver set-ups from disk.
- **Improved visualisation**: Currently, ray energies are not communicated via
  the visualisation. There is also no way of adjusting the gain of the waveguide
  visualisation, which means that often the waveguide energy dies away quickly,
  becoming invisible, and giving the false impression that the simulation is not
  progressing.
- **Command-line interface**: For scripting or batch-processing of simulations,
  it would be useful to be able to run simulations from the command-line.
  Currently, this is only made possible by writing custom wrapper programs for
  the Wayverb library. It would be more useful to integrate command-line options
  directly into the Wayverb program.

Finally, it was not possible to test the program extensively for crashes and
bugs. The program was tested on a 15-inch MacBook Pro running OS 10.11, and a
little on earlier models of 15- and 13-inch Macbook Pros running OS 10.10. On
10.11, and on the 13-inch laptop running 10.10, no crashes were evident,
although on the 15-inch 10.10 machine there were a few crashes within OpenCL
framework code. These crashes were reported by a user, from their personal
machine. There was not sufficient time to fix these bugs during the project.

Extended access to this machine was not possible, and debugging OpenCL code
without access to the problematic hardware is difficult. Depending on the
drivers supplied by the GPU vendor, the kernel may be compiled in subtly
different ways. For most (non-OpenCL) software, there will be a single binary
image for a given processor architecture, and if the program crashes then a
stack trace can be used to find the location of the bug.  However, for OpenCL
code, the executed binary is generated at runtime, and will vary depending on
the specification and driver of the GPU.  Also, crashes inside the OpenCL
kernel do not emit a stack trace.  Therefore, it is almost impossible to debug
OpenCL code without access to the specific machine configuration which causes
the issue.

A future release could fix these problems, but only with access to any
problematic hardware and software configurations. As the program is open-source
it would also be possible for third-parties experiencing bugs to contribute
fixes.

## Summary

The goal of the Wayverb project was to create a program which was capable of
simulating the acoustics of arbitrary enclosed spaces. For the program to be
useful to its target audience of musicians and sound designers, it must be
simultaneously plausible, efficient, and accessible.

The aims of plausibility and efficiency would be met by combining
wave-modelling and geometric simulation methods, benefiting from both the
physical realism of wave-modelling, and the computational performance of
geometric simulation.  This technique is not used by any other publicly
available simulation package, so it was thought that a program implementing
both models would be both faster, and produce higher-quality results, than
competing programs. To further improve performance, the simulation would be
implemented to run in parallel on graphics hardware. The program would be free
and open-source, with a graphical interface, to ensure accessibility and
encourage adoption.

Testing shows that the individual modelling methods are individually reasonably
plausible. The ray-tracing and image-source methods respond appropriately to
changes in room size, material, source/receiver spacing and receiver type.
This is also true of the waveguide, which additionally is capable of modelling
low-frequency modal responses, taking wave-effects such as diffraction into
account. However, when waveguide outputs are combined with geometric outputs,
the blended spectrum shows obvious discontinuities.  Although individual
aspects of the waveguide model (such as the boundary and receiver
implementations) perform as expected, the waveguide model as a whole does not.
The audible artefacts produced by combining the models mean that generated
results are not fit for the purposes of high-quality music production or sound
design.  That is, the project fails to achieve its principal design goal.
Future work may seek to locate errors in the implementation of the waveguide,
and to improve the match between the outputs of the different models, perhaps
sacrificing some low-frequency accuracy in the interests of sound quality.

In terms of efficiency, simulations generally complete within minutes, rather
than hours or days, meeting the project's efficiency target. It is also
possible to observe the progression of the simulation, and to retry it with
less intensive parameters, if it is progressing too slowly.  Unfortunately, the
time taken to generate outputs is not necessarily reflected in the quality of
the results.  For example, it is disappointing to wait for ten minutes for an
impulse response, only to find that the output has markedly different reverb
times at the top and bottom of the spectrum. Good user experience relies on
users being able to generate results with acceptable quality as quickly as
possible. If the user has to tweak and re-render, waiting for several minutes
each time, before eventually finding appropriate settings, this translates to a
poor user experience. This may be solved in two ways: by improving the quality
of the outputs; and/or by further optimisation of the simulation algorithms.

The application has an accessible graphical interface. Although some desirable
features (such as built-in convolution and 3D editing) are missing, the
interface is focused and functional. It is possible to install and use without
specialist training.  Additionally, all code is open-source, allowing
collaboration and contribution from interested third-parties.

On reflection, it appears that the scope of the project was too large, which in
turn meant that none of the research goals were satisfactorily achieved. The
majority of the time spent on the project was given to implementing the
program, which meant that when functional issues (like the reverb time
mismatch) were discovered during testing, there was no time to attempt a fix.
Additionally, much more testing would be necessary in order for a comprehensive
view of the program to be given, but again this was not possible.

In its current state, Wayverb is unsuitable for use in creative contexts.
However, it provides open-source implementations of three different acoustic
models, along with an intuitive graphical interface. As such, Wayverb may serve
as a solid starting point for further research in this field. It is conceivable
that a future version of Wayverb, with more comprehensive testing and
subsequent adjustment, could be a genuinely helpful tool for creative users.
