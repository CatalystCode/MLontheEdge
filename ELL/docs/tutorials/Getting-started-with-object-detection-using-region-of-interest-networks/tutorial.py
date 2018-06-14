#!/usr/bin/env python3
###############################################################################
#
#  Project:  Embedded Learning Library (ELL)
#  File:     tutorial.py
#  Authors:  Kern Handa
#
#  Requires: Python 3.x
#
###############################################################################

import cv2
import numpy as np
import tutorial_helpers as helpers

# Import the Python wrapper for the ELL model
import model

# Each pair of values represents width and height scales for each of the five
# regions that could be detected in any given cell
ANCHOR_BOXES = [1.08,1.19,  3.42,4.41,  6.63,11.38,  9.42,5.11,  16.62,10.52]

CONFIDENCE_THRESHOLD = 0.4
OVERLAP_THRESHOLD = 0.4


def get_image_from_camera(camera):
    """Read an image from the camera"""
    if camera:
        ret, frame = camera.read()
        if not ret:
            raise Exception("your capture device is not returning images")
        return frame
    return None


def main():
    camera = cv2.VideoCapture(0)

    with open("categories.txt", "r") as categories_file:
        categories = categories_file.read().splitlines()

    input_shape = model.get_default_input_shape()
    output_shape = model.get_default_output_shape()

    while (cv2.waitKey(1) & 0xFF) != 27:
        # Get the image from the camera
        image = get_image_from_camera(camera)

        # Prepare the image to pass to the model. This helper crops and resizes
        # the image maintaining proper aspect ratio and return the resultant
        # image instead of a numpy array. This is because we need to display
        # the image with the regions drawn on top. Additionally, the heper will
        # reorder the image from BGR to RGB
        image = helpers.prepare_image_for_model(
            image, input_shape.columns, input_shape.rows, reorder_to_rgb=True,
            ravel=False)

        input_data = image.astype(np.float32).ravel()

        # Get the predictions by running the model. `predictions` is returned
        # as a flat array
        predictions = model.predict(input_data)

        # Reshape the output of the model into a tensor that matches the
        # expected shape
        predictions = np.reshape(
            predictions,
            (output_shape.rows, output_shape.columns, output_shape.channels))

        # Do some post-processing to extract the regions from the output of
        # the model
        regions = helpers.get_regions(
            predictions, categories, CONFIDENCE_THRESHOLD, ANCHOR_BOXES)

        # Get rid of any overlapping regions for the same object
        regions = helpers.non_max_suppression(
            regions, OVERLAP_THRESHOLD, categories)

        # Draw the regions onto the image
        helpers.draw_regions_on_image(image, regions)

        # Display the image
        image = cv2.cvtColor(image, cv2.COLOR_RGB2BGR)
        cv2.imshow("Region detection", image)


if __name__ == "__main__":
    main()
