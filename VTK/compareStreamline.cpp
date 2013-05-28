#include <list>
#include <iterator>
#include <iostream>

#include <stdio.h>
#include <stdlib.h>


#include "vtkOSUFlow.h"
#include "vtkDataSet.h"
#include "vtkStructuredGrid.h"
#include "vtkMultiBlockPLOT3DReader.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkSmartPointer.h"
#include "vtkLineSource.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkActor.h"
#include "vtkPolyDataMapper.h"
#include "vtkStructuredGridOutlineFilter.h"
#include "vtkTesting.h"
#include "vtkProperty.h"
#include "vtkLineWidget.h"
#include "vtkCommand.h"
#include "vtkCallbackCommand.h"
// streamline
#include "vtkStreamLine.h"
#include "vtkTimerLog.h"

using namespace std;

vtkLineWidget *lineWidget;
vtkOSUFlow *streamer;
vtkStreamLine *streamer2;
vtkRenderWindow *renWin;
vtkPolyData *seeds ;
vtkTimerLog *timer = vtkTimerLog::New();

void computeStreamlines(vtkObject* caller, unsigned long eventId, void *clientdata, void *calldata)
{
	double *point1 = lineWidget->GetPoint1();
	double *point2 = lineWidget->GetPoint2();
	printf("LineWidget Point1: %lf %lf %lf, Point2: %lf %lf %lf\n", point1[0], point1[1], point1[2], point2[0], point2[1], point2[2]);


	lineWidget->GetPolyData(seeds);

	timer->StartTimer();
	streamer->Update();
	timer->StopTimer();
	printf("OSUFlow Elapsed Time: %lf\n", timer->GetElapsedTime());

	timer->StartTimer();
	streamer2->Update();
	timer->StopTimer();
	printf("vtkStreamLine Elapsed Time: %lf\n", timer->GetElapsedTime());


	renWin->Render();
	//cout << *streamer2;

}


int main(int argc, char **argv)
{
	int rakeResolution = 100;
	int steps = 100;
	float stepsize = 0.001;


	printf("Press 'i' to change the rake\n");

	// read data
	char file1[256], file2[256];
	int files;
	if (argc<=1) { // load default data
		vtkTesting *t = vtkTesting::New();
		sprintf(file1, "%s/Data/combxyz.bin", "/home/jchen/project/VTKData"); // t->GetDataRoot());
		printf("%s\n", file1);
		sprintf(file2, "%s/Data/combq.bin", "/home/jchen/project/VTKData"); //t->GetDataRoot());
		t->Delete();
		files = 2;
	} else {
		strcpy(file1, argv[1]);
		strcpy(file2, argv[2]);
	}

	// Start by loading some data.
	vtkMultiBlockPLOT3DReader *pl3dReader = vtkMultiBlockPLOT3DReader::New();
	// set data
	pl3dReader->SetXYZFileName(file1);
	pl3dReader->SetQFileName(file2);
	pl3dReader->SetScalarFunctionNumber(100);
	pl3dReader->SetVectorFunctionNumber(202);
	pl3dReader->Update();
	vtkDataSet *data;
	data = vtkDataSet::SafeDownCast(pl3dReader->GetOutput()->GetBlock(0));

	//
	// Determine seeds
	//
	// user can change the rake
	lineWidget = vtkLineWidget::New();
	lineWidget->SetInputData(data);
	lineWidget->SetResolution(rakeResolution); // 22 seeds along the line
	lineWidget->SetAlignToYAxis();
	lineWidget->PlaceWidget();
	lineWidget->ClampToBoundsOn();
	seeds = vtkPolyData::New();
	lineWidget->GetPolyData(seeds);


	//
	// vtkOSUFlow
	//
	streamer = vtkOSUFlow::New();
	streamer->SetInputData(data);
	streamer->SetSourceData(seeds);	//streamer->SetSourceConnection(rake->GetOutputPort());
	streamer->SetIntegrationStepLength(stepsize);
	streamer->SetIntegrationDirectionToForward();
	streamer->SetMaximumPropagationTime(steps);
	streamer->SetNumberOfThreads(1);
	streamer->VorticityOn();

	vtkPolyDataMapper *mapper = vtkPolyDataMapper::New();
	mapper->SetInputConnection(streamer->GetOutputPort());
	mapper->SetScalarRange(data->GetScalarRange());
	vtkActor *actor = vtkActor::New();
	actor->SetMapper(mapper);

	//
	// use vtkStreamLine
	//
	streamer2 = vtkStreamLine::New();
	streamer2->SetInputData(data);
	streamer2->SetSourceData(seeds);
	streamer2->SetMaximumPropagationTime(steps*stepsize);
	  // Description:
	  // Specify a nominal integration step size (expressed as a fraction of
	  // the size of each cell). This value can be larger than 1.
	streamer2->SetIntegrationStepLength(stepsize);
	  // Description:
	  // Specify the length of a line segment. The length is expressed in terms of
	  // elapsed time. Smaller values result in smoother appearing streamlines, but
	  // greater numbers of line primitives.
	streamer2->SetStepLength(stepsize);
	streamer2->SetNumberOfThreads(1);
	streamer2->SetIntegrationDirectionToForward();//  IntegrateBothDirections();
	streamer2->VorticityOff();

	vtkPolyDataMapper *mapper2 = vtkPolyDataMapper::New();
	mapper2->SetInputConnection(streamer2->GetOutputPort());
	mapper2->SetScalarRange((data->GetScalarRange()));
	vtkActor *actor2 = vtkActor::New();
	actor2->SetMapper(mapper2);

	//
	// outline
	//
	vtkStructuredGridOutlineFilter *outline = vtkStructuredGridOutlineFilter::New();
	outline->SetInputData(data);

	vtkPolyDataMapper *outlineMapper = vtkPolyDataMapper::New();
	outlineMapper->SetInputConnection(outline->GetOutputPort());

	vtkActor *outlineActor = vtkActor::New();
	outlineActor->SetMapper(outlineMapper);
	outlineActor->GetProperty()->SetColor(0,0,0);


	//
	// renderer
	//
	vtkRenderer *ren = vtkRenderer::New();
	renWin = vtkRenderWindow::New();
	renWin->AddRenderer(ren);
	vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
	iren->SetRenderWindow(renWin);
	vtkInteractorStyleTrackballCamera *style = vtkInteractorStyleTrackballCamera::New();
	iren->SetInteractorStyle(style);


	// line widget interactor
	lineWidget->SetInteractor(iren);
	lineWidget->SetDefaultRenderer(ren);
	vtkCallbackCommand *callback = vtkCallbackCommand::New();
	callback->SetCallback(computeStreamlines);
	lineWidget->AddObserver(vtkCommand::EndInteractionEvent, callback);

	//ren->AddActor(rakeActor);
	ren->AddActor(actor);
	ren->AddActor(actor2);
	ren->AddActor(outlineActor);
	ren->SetBackground(.5,.5,.5);

	renWin->SetSize(500,500);

	iren->Initialize();
	renWin->Render();
	iren->Start();

	return 0;
}



