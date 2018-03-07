/*=========================================================================
 *
 *  Copyright Insight Software Consortium
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *         http://www.apache.org/licenses/LICENSE-2.0.txt
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *=========================================================================*/
#ifndef itkPhaseCorrelationImageRegistrationMethod_h
#define itkPhaseCorrelationImageRegistrationMethod_h

#include "itkImage.h"
#include "itkProcessObject.h"
#include <complex>
#include "itkConstantPadImageFilter.h"
#include "itkRealToHalfHermitianForwardFFTImageFilter.h"
#include "itkHalfHermitianToRealInverseFFTImageFilter.h"
#include "itkDataObjectDecorator.h"
#include "itkTranslationTransform.h"
#include "itkCropImageFilter.h"

#include "itkPhaseCorrelationOperator.h"
#include "itkPhaseCorrelationOptimizer.h"

namespace itk
{

/** \class PhaseCorrelationImageRegistrationMethod
 *  \brief Base class for phase-correlation-based image registration.
 *
 *  Phase Correlation Method (PCM) estimates shift between the Fixed image and
 *  Moving image. See <em>C. D. Kuglin and D. C. Hines, The phase correlation
 *  image alignment method, in Proc. Int. Conf. on Cybernetics and Society,
 *  pp. 163-165, IEEE, Sep. 1975</em> for method description.
 *
 *  The method consists of 4 (5) steps:
 *    0. Padding or resampling the images to the same spacing and size.
 *    1. Compute FFT of the two images.
 *    2. Compute the ratio of the two spectrums.
 *    3. Compute the inverse FFT of the cross-power spectrum.
 *    4. Find the maximum peak in cross-power spectrum and estimate the shift.
 *
 *  Step 0. is not included in the method itself - it is a prerequisite of PCM.
 *  It is required that the input itk::Image's have the same Spacing and
 *  Direction. If that is not the case, resample one of the images with the
 *  ResampleImageFilter prior to applying this method.
 *  This class will pad the smaller image by zeros by default to obtain two images
 *  that have the same real size (in all dimensions). Or, it will pad both
 *  images to the size specified with PadToSize, if set.
 *
 *  Step 1. is performed by this class too using FFT filters supplied by
 *  itk::RealToHalfHermitianForwardFFTImageFilter::New() factory.
 *
 *  Step 2. is performed by generic PhaseCorrelationOperator supplied at
 *  run-time.  PhaseCorrelationOperator can be derived to implement some special
 *  filtering during this phase.
 *
 *  As some special techniques (e.g. to compute subpixel shifts) require complex
 *  correlation surface, while the others compute the shift from real
 *  correlation surface, step 3. is carried by this class only when necessary.
 *  The IFFT filter is created using
 *  itk::HalfHermitianToRealInverseFFTImageFilter::New() factory.
 *
 *  Step 4. is performed with the run-time supplied PhaseCorrelationOptimizer. It has
 *  to determine the shift from the real or complex correlation surface.
 *
 *
 *  First, plug in the operator, optimizer and the input images. The method
 *  is executed by calling Update() (or updating some downstream filter).
 *
 *  The output shift can be passed downstreams in the form of
 *  TranslationTransform or can be obtained as transform parameters vector. The
 *  transform can be directly used to resample the Moving image to match the
 *  Fixed image.
 *
 * \author Jakub Bican, jakub.bican@matfyz.cz, Department of Image Processing,
 *         Institute of Information Theory and Automation,
 *         Academy of Sciences of the Czech Republic.
 *
 * \ingroup Montage
 */
template <typename TFixedImage, typename TMovingImage>
class ITK_TEMPLATE_EXPORT PhaseCorrelationImageRegistrationMethod: public ProcessObject
{
public:
  /** Standard class typedefs. */
  typedef PhaseCorrelationImageRegistrationMethod Self;
  typedef ProcessObject                           Superclass;
  typedef SmartPointer<Self>                      Pointer;
  typedef SmartPointer<const Self>                ConstPointer;

  /** Method for creation through the object factory. */
  itkNewMacro(Self);

  /** Run-time type information (and related methods). */
  itkTypeMacro(PhaseCorrelationImageRegistrationMethod, ProcessObject);

  /**  Type of the Fixed image. */
  typedef          TFixedImage                     FixedImageType;
  typedef typename FixedImageType::PixelType       FixedImagePixelType;
  typedef typename FixedImageType::ConstPointer    FixedImageConstPointer;

  /**  Type of the Moving image. */
  typedef          TMovingImage                    MovingImageType;
  typedef typename MovingImageType::ConstPointer   MovingImageConstPointer;

  /** Dimensionality of input and output data is assumed to be the same. */
  itkStaticConstMacro( ImageDimension, unsigned int, FixedImageType::ImageDimension );

  /** Pixel type, that will be used by internal filters.
   *  It should be float for integral and float inputs and it should
   *  be double for double inputs */
  typedef typename NumericTraits<FixedImagePixelType>::RealType InternalPixelType;

  /** Type of the image, that is passed between the internal components. */
  typedef Image< InternalPixelType, ImageDimension > RealImageType;

  /** Type of the image, that is passed between the internal components. */
  typedef Image< std::complex< InternalPixelType >,
                               itkGetStaticConstMacro(ImageDimension) >
                                                   ComplexConjugateImageType;

  /**  Type of the Operator */
  typedef          PhaseCorrelationOperator< InternalPixelType, ImageDimension > OperatorType;
  typedef typename OperatorType::Pointer                                         OperatorPointer;

  /**  Type of the Optimizer */
  typedef          PhaseCorrelationOptimizer< RealImageType >
                                                   RealOptimizerType;
  typedef typename RealOptimizerType::Pointer      RealOptimizerPointer;
  typedef          PhaseCorrelationOptimizer< ComplexConjugateImageType >
                                                   ComplexOptimizerType;
  typedef typename ComplexOptimizerType::Pointer   ComplexOptimizerPointer;

  /**  Type for the transform. */
  typedef          TranslationTransform<
                           typename MovingImageType::PointType::ValueType,
                           ImageDimension >
                                                   TransformType;
  typedef typename TransformType::Pointer          TransformPointer;

  /** Type for the output transform parameters (the shift). */
  typedef typename TransformType::ParametersType   ParametersType;

  /** Type for the output: Using Decorator pattern for enabling
   *  the Transform to be passed in the data pipeline */
  typedef  DataObjectDecorator< TransformType >    TransformOutputType;
  typedef typename TransformOutputType::Pointer    TransformOutputPointer;
  typedef typename TransformOutputType::ConstPointer
                                                   TransformOutputConstPointer;

  /** Smart Pointer type to a DataObject. */
  typedef typename DataObject::Pointer DataObjectPointer;

  /** Set/Get the Fixed image. */
  void SetFixedImage( const FixedImageType * fixedImage );
  itkGetConstObjectMacro( FixedImage, FixedImageType );

  /** Set/Get the Moving image. */
  void SetMovingImage( const MovingImageType * movingImage );
  itkGetConstObjectMacro( MovingImage, MovingImageType );

  /** Passes ReleaseDataFlag to internal filters. */
  void SetReleaseDataFlag(bool flag) override;

  /** Passes ReleaseDataBeforeUpdateFlag to internal filters. */
  void SetReleaseDataBeforeUpdateFlag(const bool flag) override;

  /** Set/Get the Operator. */
  itkSetObjectMacro( Operator, OperatorType );
  itkGetConstObjectMacro( Operator, OperatorType );

  /** Set/Get the Optimizer. */
  virtual void SetOptimizer (RealOptimizerType *);
  virtual void SetOptimizer (ComplexOptimizerType *);
  itkGetConstObjectMacro( RealOptimizer,  RealOptimizerType );
  itkGetConstObjectMacro( ComplexOptimizer,  ComplexOptimizerType );

  /** Get the correlation surface.
   *
   *  Use method appropriate to the type (real/complex) of optimizer. If the
   *  complex optimizer is used, the real correlation surface is not available
   *  or is not up-to-date.
   */
  virtual RealImageType * GetRealCorrelationSurface()
    {
    itkDebugMacro("returning RealCorrelationSurface address "
                  << this->m_IFFT->GetOutput() );
    if (m_IFFT.IsNotNull())
      {
      return m_IFFT->GetOutput();
      }
    else
      {
      return 0;
      }
    }
  virtual ComplexConjugateImageType * GetComplexCorrelationSurface()
    {
    itkDebugMacro("returning ComplexCorrelationSurface address "
                  << this->m_Operator->GetOutput() );
    if (m_Operator.IsNotNull())
      {
      return m_Operator->GetOutput();
      }
    else
      {
      return 0;
      }
    }

  /** Get the computed transformation parameters. */
  itkGetConstReferenceMacro( TransformParameters, ParametersType );

  /** Returns the transform resulting from the registration process  */
  const TransformOutputType * GetOutput() const;

  /** Returns the phase correlation image from the registration process  */
  const RealImageType * GetPhaseCorrelationImage() const;

#ifdef ITK_USE_CONCEPT_CHECKING
  itkStaticConstMacro(MovingImageDimension, unsigned int,
    FixedImageType::ImageDimension );
  /** Start concept checking */
  itkConceptMacro(SameDimensionCheck,
    (Concept::SameDimension<ImageDimension, MovingImageDimension>));
#endif

protected:
  PhaseCorrelationImageRegistrationMethod();
  virtual ~PhaseCorrelationImageRegistrationMethod() {};
  void PrintSelf(std::ostream& os, Indent indent) const override;

  /** Make a DataObject of the correct type to be used as the specified
   * output. */
  virtual DataObjectPointer MakeOutput(DataObjectPointerArraySizeType idx) override;

  /** Initialize by setting the interconnects between the components. */
  virtual void Initialize();

  /** Determine the correct padding for the fixed image and moving image. */
  void DeterminePadding();

  /** Method that initiates the optimization process. */
  void StartOptimization();

  /** Method invoked by the pipeline in order to trigger the computation of
   * the registration. */
  void GenerateData() override;

  /** Method invoked by the pipeline to determine the output information. */
  void GenerateOutputInformation() override;

  /** Provides derived classes with the ability to set this private var */
  itkSetMacro( TransformParameters, ParametersType );


  /** Types for internal componets. */
  typedef ConstantPadImageFilter< FixedImageType, RealImageType >                     FixedPadderType;
  typedef ConstantPadImageFilter< MovingImageType, RealImageType >                    MovingPadderType;
  typedef RealToHalfHermitianForwardFFTImageFilter< RealImageType >                   FFTFilterType;
  typedef typename FFTFilterType::OutputImageType                                     ComplexImageType;
  typedef HalfHermitianToRealInverseFFTImageFilter< ComplexImageType, RealImageType > IFFTFilterType;
  typedef CropImageFilter< RealImageType, RealImageType >                             CropFilterType;

private:
  ITK_DISALLOW_COPY_AND_ASSIGN(PhaseCorrelationImageRegistrationMethod);

  OperatorPointer                       m_Operator;
  RealOptimizerPointer                  m_RealOptimizer;
  ComplexOptimizerPointer               m_ComplexOptimizer;

  MovingImageConstPointer               m_MovingImage;
  FixedImageConstPointer                m_FixedImage;

  ParametersType                        m_TransformParameters;

  typename FixedPadderType::Pointer     m_FixedPadder;
  typename MovingPadderType::Pointer    m_MovingPadder;
  typename FFTFilterType::Pointer       m_FixedFFT;
  typename FFTFilterType::Pointer       m_MovingFFT;
  typename IFFTFilterType::Pointer      m_IFFT;
  typename CropFilterType::Pointer      m_CropFilter;

};

} // end namespace itk


#ifndef ITK_MANUAL_INSTANTIATION
#include "itkPhaseCorrelationImageRegistrationMethod.hxx"
#endif

#endif
