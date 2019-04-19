/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Driver.h
 * Author: linux
 *
 * Created on February 9, 2019, 5:45 PM
 */

#ifndef DRIVER_H
#define DRIVER_H

#include "DandereUtils/DandereUtils.h" //waitForFile
#include "Difference/PDifference.h"
#include "Difference/Correction.h"

/**
 * 
 * 1) In Dandere2x, the first frame of any function is seen as different.
 * Reiterating, parts of frame1 can exists in parts of frame2, frame5, or frame1000.
 * 
 * That is to say, every frame preceding frame1 can possibely have parts derived
 * from frame1. As a result, we load frame1 seperate from the rest,
 * and continually make modifications as we go.
 * 
 * 
 * 2) Given two frames and the data required to start finding differences, 
 * compute the differences between two frames. 
 * 
 * 3) Read comment method for this. We need to draw over the previous frame
 * 
 * 4) Assign drawn over frame as the new base frame. 
 * 
 * @param workspace
 * @param frameCount
 * @param blockSize
 * @param tolerance
 * @param stepSize
 */

const int correctionBlockSize = 4;

void driverDifference(
        std::string workspace,
        int resumeCount, 
        int frameCount,
        int blockSize,
        double tolerance,
        double mse_max,
        double mse_min,
        int stepSize,
        std::string extensionType) {


    int bleed = 2; //i dont think bleed is actually used? 
    bool debug = true;

    //1 
    waitForFile(workspace + separator() + "inputs" + separator() + "frame" + to_string(1) + extensionType);
    shared_ptr<Image> im1 = make_shared<Image>(workspace + separator() + "inputs" + separator() + "frame" + to_string(1) + extensionType);

    
    //for resume case, we need to manually process this frame in a different manner.
    if (resumeCount != 1) {
        shared_ptr<Image> im2 = make_shared<Image>(workspace + separator() + "inputs" + separator() + "frame" + to_string(resumeCount + 1) + extensionType);
        //File locations that will be produced 
        std::string pDataFile = workspace + separator() + "pframe_data" + separator() + "pframe_" + std::to_string(resumeCount) + ".txt";
        std::string inversionFile = workspace + separator() + "inversion_data" + separator() + "inversion_" + std::to_string(resumeCount) + ".txt";
        std::string correctionFile1 = workspace + separator() + "correction_data" + separator() + "correction_" + std::to_string(resumeCount) + ".txt";
        writeEmpty(pDataFile);
        writeEmpty(inversionFile);
        writeEmpty(correctionFile1);
        im1 = im2;

        resumeCount++;
    }

    for (int x = resumeCount; x < frameCount; x++) {
        std::cout << "Computing differences for frame" << x << endl;

        //Load Images for this iteration
        waitForFile(workspace + separator() + "inputs" + separator() + "frame" + to_string(x + 1) + extensionType);
        shared_ptr<Image> im2 = make_shared<Image>(workspace + separator() + "inputs" + separator() + "frame" + to_string(x + 1) + extensionType);
        shared_ptr<Image> copy = make_shared<Image>(workspace + separator() + "inputs" + separator() + "frame" + to_string(x + 1) + extensionType); //for psnr
        
        //File locations that will be produced 
        std::string pDataFile = workspace + separator() + "pframe_data" + separator() + "pframe_" + std::to_string(x) + ".txt";
        std::string inversionFile = workspace + separator() + "inversion_data" + separator() + "inversion_" + std::to_string(x) + ".txt";
        std::string correctionFile1 = workspace + separator() + "correction_data" + separator() + "correction_" + std::to_string(x) + ".txt";

        PDifference dif = PDifference(im1, im2, blockSize, bleed, tolerance, pDataFile, inversionFile, stepSize, debug);
        dif.generatePData(); //2
        dif.drawOver(); //3
   
        int correction_factor = sqrt(blockSize) / sqrt(correctionBlockSize); // not rly sure why this works tbh
        Correction cor1 = Correction(im2, copy, correctionBlockSize, bleed, tolerance * correction_factor, correctionFile1, correctionBlockSize, true);
        cor1.matchAllBlocks();
        cor1.drawOver();
        
        double p_mse = ImageUtils::mse_image(*im2, *copy);
        std::cout << "p_frame " << x << "mse: " << p_mse << endl;
        
        if (p_mse > mse_max && tolerance > 1) {
            std::cout << "mse too high " << p_mse << " > " << mse_max << std::endl;
            std::cout << "Changing Tolerance " << tolerance << " -> " << tolerance - 1 << std::endl;
            tolerance--;
            x--;
            continue; //redo this current for loop iteration with different settings
        }

        if (p_mse < mse_min && tolerance < 30) {
            std::cout << "mse too too low: " << p_mse << " < " << mse_min << std::endl;
            std::cout << "Changing Tolerance " << tolerance << " -> " << tolerance + 1 << std::endl;
            tolerance++;
        }

        dif.save();
        cor1.writeCorrection();
        im1 = im2; //4
    }
}
#endif /* DRIVER_H */

