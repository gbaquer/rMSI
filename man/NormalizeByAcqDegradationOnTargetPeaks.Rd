% Generated by roxygen2: do not edit by hand
% Please edit documentation in R/normalizations.R
\name{NormalizeByAcqDegradationOnTargetPeaks}
\alias{NormalizeByAcqDegradationOnTargetPeaks}
\title{NormalizeByAcqDegradationOnTargetPeaks: Calculates the normalization of each pixel as the sum of areas of specified peaks.
Each normalization coeficient is smoothed by its neighbours to compensate for ionization degradation during acqusition.}
\usage{
NormalizeByAcqDegradationOnTargetPeaks(
  img,
  mz_vector,
  mz_window_size = 0.1,
  norm_name = "AcqSelPK",
  winSize = 0.1
)
}
\arguments{
\item{img}{the rMSI image object.}

\item{mz_vector}{a vector containing m/z values of peaks to be used for normalization.}

\item{mz_window_size}{specifies the with of peak gaussian. Set to 0.1 Da by default which is OK for reflector TOF MALDI instrument}

\item{norm_name}{a name to be used for the normalization. Default is set to "SelPK".}

\item{winSize}{the window size use for smoothing (0 to 1).}
}
\value{
a rMSI image containing the normalizations$<name> field
}
\description{
NormalizeByAcqDegradationOnTargetPeaks: Calculates the normalization of each pixel as the sum of areas of specified peaks.
Each normalization coeficient is smoothed by its neighbours to compensate for ionization degradation during acqusition.
}
