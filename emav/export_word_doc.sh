#!/bin/bash
# Run this script whenever you update UserGuide.md to generate a fresh Word document

echo "Converting UserGuide.md to Word Document..."
pandoc UserGuide.md -o ../../../tm/EMAV_UserGuide.docx
echo "Done! The updated Word document is located at STN/tm/EMAV_UserGuide.docx"
