% Generated by roxygen2: do not edit by hand
% Please edit documentation in R/imzMLreader.R
\name{fixImzMLDuplicates}
\alias{fixImzMLDuplicates}
\title{fixImzMLDuplicates.
Delete duplicates and possible zero-drop errors (fixing Bruker's bugs in imzML).}
\usage{
fixImzMLDuplicates(mass, intensity)
}
\arguments{
\item{mass}{spectrum mass axis.}

\item{intensity}{spectrum intensity.}
}
\value{
a list with non-duplicated mass and intensity vectors.
}
\description{
fixImzMLDuplicates.
Delete duplicates and possible zero-drop errors (fixing Bruker's bugs in imzML).
}
