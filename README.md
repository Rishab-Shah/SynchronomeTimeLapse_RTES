# SynchronomeTimeLapse_RTES
ECEN 5863: RTES : Summer 2021 : Synchronome Time Lapse Project

### Project Description
An external clock – Physical or https://www.visnos.com/demos/clock would be used to acquire synchronized images where all images would show a unique state of hour, minute and second hands on an ANALOG_CLOCK using a resolution of VGA 640x840. Based on this resolution, accurate time-lapse images will be encoded. An effort to achieve 1 Hz verification (no skips, blurs, or repeats) with an ANALOG CLOCK is to be completed and full-credit STRETCH goal for 10 Hz verification with a DIGITAL STOPWATCH over 1801 frames (30 minutes for 1 Hz, 3 minute for 10 Hz).  

#### Block Diagram
![HighLevelBlockDiagram](https://user-images.githubusercontent.com/77663015/170643665-58bcbbfb-e134-4af3-85fb-4bedf1ad58a4.png)

#### Software Architecture
![10_Hz_SoftwareArch](https://user-images.githubusercontent.com/77663015/170643681-25d71028-e42b-4ec1-bb7c-64581584ef2c.png)

### Video::
* To be uploaded

### Analysis/Output:
#### 1 Hz WCET Analysis
![1_Hz_ImageAcq_service](https://user-images.githubusercontent.com/77663015/170644147-f6326eca-66c7-4ff7-8d5b-ea7b7c8c0765.png)

![1_Hz_WCETAnalysis](https://user-images.githubusercontent.com/77663015/170643990-e7ee672e-6ef1-4865-9f75-46705a6fb534.png)

#### 10 Hz WCET Analysis
![10_Hz_ImageAcqService](https://user-images.githubusercontent.com/77663015/170644000-1fba229a-1e0f-4cc3-92bb-c1a5ce14415d.png)  

![10_Hz_WCETAnalysis](https://user-images.githubusercontent.com/77663015/170644210-f2612d26-bcc1-4da2-979e-3a508e8e4428.png)  


### Toggle Output:
![Toggle Negative Transformation](https://user-images.githubusercontent.com/77663015/170644356-54e8377a-2a7a-48d9-8f93-d73b4c5ef38f.png)


### Quick commands:
* Please use `help` command to understand to run the synchronome time lapse capture at different frequency and toggle functionality

### File description
* `main.c,main.h:` This file consists if initialization of all the modules required in the project  
* `camera_driver.c,camera_driver.h:` The device driver which interacts with Logitech USB camera
* `services.c,services.h:` All the services such as Image Acq., Sequencer, YUYV to RGB, RGB to Negative, Dumping to memory etc.
* `time_spec.c,time_spec.h:` Time logging functionalities

### Environment
* Raspbian OS  
* Board/Hardware: Rpi-4  

### Author
* Rishab Shah

### References
[1] https://en.wikipedia.org/wiki/Shortt%E2%80%93Synchronome_clock  
[2]The starter code http://ecee.colorado.edu/~ecen5623/ecen/ex/Linux/code/Std-Project-Starter-Code/ was used for the project (V4L2)  

The following video and links were consulted to get an understanding of the software design.  
[3]http://ecee.colorado.edu/~ecen5623/ecen/video/lecture/Coursera-rough-cuts/RTES-Example-RT-Synchronome-1-Hz-and-10-Hz-Demo/  
[4]http://ecee.colorado.edu/~ecen5623/ecen/video/lecture/Coursera-rough-cuts/RTES-Demo-Full-Final-Cut/  
[5]http://ecee.colorado.edu/~ecen5623/ecen/video/lecture/Coursera-rough-cuts/RTES-Demo-10-Hz-Demo-Only-Final-Cut/  
[6]http://ecee.colorado.edu/~ecen5623/ecen/video/lecture/Coursera-rough-cuts/RTES-Demo-1-OpenCV-Diff-Interactive/  
[7]http://ecee.colorado.edu/~ecen5623/ecen/video/lecture/Coursera-rough-cuts/RTES-Demo-1-Hz-Demo-Only/  
[8] https://www.epochconverter.com/  


