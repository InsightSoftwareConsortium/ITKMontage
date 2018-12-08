set(Montage_SRC
  itkParseTileConfiguration.cxx
  )

add_library(Montage ${Montage_SRC})
# target_link_libraries(Montage ${ITKCommon}) # we do not link-depend on anything
itk_module_target(Montage)
