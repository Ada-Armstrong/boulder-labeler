#include <opencv2/opencv.hpp>
#include <vector>
#include <string>
#include <filesystem>

using namespace cv;

enum Brush {
	ONE = 0,
	TWO,
	THREE,
	FOUR,
	FIVE,
	NUM_BRUSHES // should always be last
};

// configuration of all program parameters
struct Config {
	Mat image;
	std::vector<Mat> masks;
	Rect bounding_box;
	Brush brush;
	std::vector<Rect> bounding_boxes[NUM_BRUSHES]; // array of vectors

	std::vector<Rect> &get_current_bb()
	{
		return bounding_boxes[brush];
	}
};

// ***** GLOBALS *****
const char *window_name = "editor";

Scalar RED{0, 0, 255, 255};
Scalar GREEN{0, 255, 0, 255};
Scalar BLUE{255, 0, 0, 255};
Scalar YELLOW{0, 255, 255, 255};
Scalar PURPLE{255, 0, 255, 255};

Scalar BRUSH_COLOURS[] = {RED, GREEN, BLUE, YELLOW, PURPLE};

// ***** FUNCTION DECLERATIONS *****
int label_problems(const char *file_name, std::string output_dir);
void mouseHandler(int event, int x, int y, int, void *config);
void draw_bounding_boxes(Mat &image, std::vector<Rect> *bounding_boxes);
Mat create_mask(Mat &image, std::vector<Rect> &bounding_boxes);
std::vector<Mat> create_masks(Mat &image, std::vector<Rect> *bounding_boxes);


int main(int argc, char **argv)
{
	const std::string about =
		"\nThis is a program to create bit mask images from a reference image\n"
		"using user inputted bounding boxes. Multiple bit masks can be created\n"
		"at once using different \'brushes\'. Each brush will output a\n"
		"separate (disjoint) bit mask image once saved. All arguments besides\n"
		"the ones listed below are treated as file pathes to images.\n"
		;
	const std::string keys = 
		"\nUsage: BoulderLabeler\n"
		"\t-h\t--help\t\t: display this help message\n"
		"\t-o\t--output\t: the output directory to save files to\n"
		;
	const std::string hot_keys =
		"\nKey binds:\n"
		"\tq\t: quit the current image without saving\n"
		"\ts\t: save the bit mask images to the output dir\n"
		"\tu\t: undo the last selection for the current brush\n"
		"\t1-5\t: change the brush\n"
		;
		
	if (argc < 2) {
		std::cout << "Incorrect number of command line arguments. See help for usage." << std::endl;
		return -1;
	}

	std::string output_dir = ".";
	std::vector<std::string> file_names;

	for (int i = 1; i < argc; ++i) {
		std::string arg(argv[i]);
		if (arg == "-h" || arg == "--help") {
			std::cout << about << keys << hot_keys;
			return 0;
		} else if (arg == "-o" || arg == "-output") {
			output_dir = std::string(argv[++i]);
		} else {
			file_names.emplace_back(arg);
		}
	}

	for (auto &file_name : file_names) {
		label_problems(file_name.c_str(), output_dir);
	}

	return 0;
}

int label_problems(const char *file_name, std::string output_dir)
{
	std::string fname = std::filesystem::path(file_name).stem(); // used for output files

	Config config;
	config.brush = ONE;

	config.image = imread(file_name, 1);

	if (!config.image.data) {
		std::cout << "No image data for " << file_name << ", skipping..." << std::endl;
		return -1;
	}

	namedWindow(window_name, WINDOW_AUTOSIZE);
	imshow(window_name, config.image);
	
	setMouseCallback(window_name, mouseHandler, &config);

	while (1) {
		int key = waitKey(0);

		switch (key) {
			case 'q':
				// quit
				std::cout << "QUIT without saving: " << file_name << std::endl;
				return 0;
			case 'u':
				// undo
				if (config.get_current_bb().size()) {
					std::cout << "UNDO for brush: " << (int)config.brush << std::endl;
					config.get_current_bb().pop_back();
					draw_bounding_boxes(config.image, config.bounding_boxes);
				}
				break;
			case 's':
				// save and quit
				// create binary image from bounding boxes
				config.masks = create_masks(config.image, config.bounding_boxes);
				// then save the images to a files
				for (int i = 0; i < config.masks.size(); ++i) {
					imwrite(output_dir + "/" + fname + "_output" + std::to_string(i) + ".jpg", config.masks[i]);
				}
				std::cout << "SAVED " << config.masks.size() << " outputs for " << file_name << std::endl;
				return 0;
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
				// change the 'brush' to the corresponding value
				config.brush = Brush(key - '1');
				std::cout << "Changed brush to: " << (int)config.brush << std::endl;
				break;
			default:
				break;
		}
	}

	return 0;
}

void mouseHandler(int event, int x, int y, int, void *config)
{
	Config *conf = (Config *)config;

	if (event == EVENT_LBUTTONDOWN) {
		conf->bounding_box.x = x;
		conf->bounding_box.y = y;
	} else if (event == EVENT_LBUTTONUP) {
		int width = x - conf->bounding_box.x;
		int height = y - conf->bounding_box.y;

		if (width <= 0 || height <= 0) {
			return;
		}

		conf->bounding_box.width = width;
		conf->bounding_box.height = height;

		conf->get_current_bb().push_back(conf->bounding_box);
		std::cout << "[BRUSH ID: " << (int)conf->brush << "]"
			<< " New Rect: " << conf->bounding_box.width << ", " << conf->bounding_box.height
			<< " <LEN BRUSH: " << conf->get_current_bb().size() << ">" << std::endl;

		draw_bounding_boxes(conf->image, conf->bounding_boxes);
	}
}

void draw_bounding_boxes(Mat &image, std::vector<Rect> *bounding_boxes)
{
	Mat copy = image.clone();
	const int thickness = 3;

	for (int i = 0; i < NUM_BRUSHES; ++i) {
		for (auto &rect : bounding_boxes[i]) {
			rectangle(copy, rect, BRUSH_COLOURS[i], thickness);
		}
	}

	imshow(window_name, copy);
}

Mat create_mask(Mat &image, std::vector<Rect> &bounding_boxes)
{
	Mat mask = Mat::zeros(image.rows, image.cols, CV_8U);

	for (auto &rect : bounding_boxes) {
		mask(rect) = 255;
	}

	return mask;
}

std::vector<Mat> create_masks(Mat &image, std::vector<Rect> *bounding_boxes)
{
	std::vector<Mat> out;

	for (int i = 0; i < NUM_BRUSHES; ++i) {
		if (bounding_boxes[i].size()) {
			out.emplace_back(create_mask(image, bounding_boxes[i]));
		}
	}

	return out;
}
