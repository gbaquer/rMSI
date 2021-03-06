% Generated by roxygen2: do not edit by hand
% Please edit documentation in R/librMSIdata.R
\name{PlotTICImage}
\alias{PlotTICImage}
\title{PlotTICImage.}
\usage{
PlotTICImage(
  img,
  TICs = NULL,
  rotate = 0,
  scale_title = "TIC",
  vlight = 5,
  labels_x = NULL,
  labels_y = NULL,
  labels_text = NULL
)
}
\arguments{
\item{img}{an rMSI object.}

\item{TICs}{a vector of TIC values ordered acording pos array in img object.}

\item{rotate}{image rotation in degrees, possible values are: 0, 90, 180 and 270.}

\item{scale_title}{the title to show in scale axis (TIC by default).}

\item{vlight}{the lighting of the plotted image.}

\item{labels_x}{x coordinates of text labels optionally overlaid to the plot.}

\item{labels_y}{y coordinates of text labels optionally overlaid to the plot.}

\item{labels_text}{text labels optionally overlaid to the plot.}
}
\description{
PlotTICImage.
}
