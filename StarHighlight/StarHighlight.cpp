#include "lodepng.h"
#include <iostream>
#include <string>

class Error
{
private:
	std::string message;
public:
	Error(std::string message): message(message){}
	virtual std::string what()
	{ 
		return message; 
	}
};

class NameError : public Error
{
private:
	std::string type;
public:
	NameError(std::string message):Error(message){}
	std::string what() override
	{
		return "Name Error!" + Error::what();
	}
};

class ValueError : public Error
{
private:
	std::string type;
public:
	ValueError(std::string message) :Error(message) {}
	std::string what() override
	{
		return "Incorrect input value. " + Error::what();
	}
};

class MemoryError : public Error
{
private:
	std::string type;
public:
	MemoryError(std::string message) :Error(message) {}
	std::string what() override
	{
		return "Memory error! " + Error::what();
	}
}; 


class Presenter {
private:
	int* color;
public:
	void set_input_file(std::string& filename)
	{
		std::cout << "Please enter the name of input file: " << std::endl;
		std::cin >> filename;
	}
	void set_output_file(std::string& filename)
	{
		std::cout << "Please enter the name of output file: " << std::endl;
		std::cin >> filename;
	}
	int* set_color()
	{
		int R, G, B;
		std::cout << "Please set the color of stars. Enter 3 values from 0 to 255";
		std::cin >> R >> G >> B;
		if (R < 0 || G < 0 || B < 0 || R > 255 || G > 255 || B > 255)
			throw ValueError("the color values should be from 0 to 255!");
		int* arr = new int[3];
		arr[0] = R;
		arr[1] = G;
		arr[2] = B;
		this->color = 0;
		return arr;
	}
	~Presenter()
	{
		delete[] color;
	}
};

class Input {
	friend class ShotHandler;
private:
	unsigned int width;
	unsigned int height;
	const char* path;
	unsigned char* picture;
	unsigned char& operator[](int index)
	{
		return picture[index];
	}
public:
	Input(const char* path) 
		:path(path)
	{
		unsigned char* picture = nullptr;
		int error = lodepng_decode32_file(&picture, &width, &height, path);
		if (error) {
			throw
	NameError("Incorrect file name or forbidded symbols were used!Please recheck the input!");
		}
		this->picture = picture;
	}

	inline unsigned int GetWidth() const { return width; }
	inline unsigned int GetHeigth() const { return height; }
};


class ShotHandler {
private:
	struct pixel {
		char R;
		char G;
		char B;
		char alpha;
	};

	struct mst_node {
		mst_node* p;

		unsigned char input_a;
		double output_r;
		double output_g;
		double output_b;

		int rank;
		int size;
	};


	mst_node** nod;

	Input in;
	unsigned char* cw;
	unsigned int* coords;

	mst_node* make_set(double R, double G, double B, unsigned char cur_alpha)
	{
		mst_node* elem = new mst_node[sizeof(mst_node)];
		if (elem == nullptr)
			throw MemoryError("Can't alocate the memory!");
		elem->p = new mst_node[sizeof(mst_node)];
		elem->p = elem;
		elem->output_r = R;
		elem->output_g = G;
		elem->output_b = B;
		elem->input_a = cur_alpha;
		elem->rank = 0;
		elem->size = 1;
		return elem;
	}

	mst_node* find_set(mst_node* elem)
	{
		if (elem->p == elem)
			return elem;
		return elem->p = find_set(elem->p);
	}

	void union_sets(mst_node* elem1, mst_node* elem2)
	{
		elem1 = find_set(elem1);
		elem2 = find_set(elem2);

		if (elem1 == elem2) return;

		if (elem1->rank < elem2->rank)
		{
			elem2->input_a = elem1->input_a;

			elem2->output_r = elem1->output_r;
			elem2->output_g = elem1->output_g;
			elem2->output_b = elem1->output_b;

			elem2->size += elem1->size;
			elem1->p = elem2;
		}
		else
		{
			elem1->input_a = elem2->input_a;

			elem1->output_r = elem2->output_r;
			elem1->output_g = elem2->output_g;
			elem1->output_b = elem2->output_b;

			elem1->size += elem2->size;
			elem2->p = elem1;
			if (elem1->rank == elem2->rank)
				++elem2->rank;
		}
	}

public:
	ShotHandler(Input& _in):in(_in), coords(nullptr)
	{
		unsigned int width = in.GetWidth();
		unsigned int height = in.GetHeigth();
		unsigned char* cw = new unsigned char[4 * width * height * sizeof(unsigned char)];
		this->cw = cw;

		mst_node** nod = new mst_node* [(width * height) * sizeof(mst_node*)];
		this->nod = nod;
	}

	~ShotHandler()
	{
		delete[] cw;
		for (int i = 0; i < in.GetWidth() * in.GetHeigth(); ++i)
			delete[] nod[i];
		delete[] nod;
		delete[] coords;
	}

	void discolor()
	{
		for (int i = 0; i < 4 * in.GetWidth() * in.GetHeigth(); i += 4) {
			struct pixel P;
			P.R = in[i];
			P.G = in[i + 1];
			P.B = in[i + 2];
			P.alpha = in[i + 3];
			cw[i] = 255;
			cw[i + 1] = 255;
			cw[i + 2] = 255;
			cw[i + 3] = ((P.R + P.G + P.B + P.alpha) / 4) % 256;
		}
	}

	void create_graph()
	{
		unsigned char R, G, B;
		for (int i = 0; i < in.GetHeigth(); ++i)
		{
			for (int j = 0; j < in.GetWidth(); ++j)
			{
				R = rand() % 256;
				G = rand() % 256;
				B = rand() % 256;
				nod[i * in.GetWidth() + j] = make_set(R, G, B, 
					cw[4 * (i * in.GetWidth() + j) + 3]);
			}
		}
	}

	void remove_noises(int lower_bound, int upper_bound)
	{
		if (lower_bound < 0 || upper_bound < 0 ||
			lower_bound > 255 || upper_bound > 255 || lower_bound >= upper_bound)
			throw ValueError("the color values should be from 0 to 255!");
		for (int i = 0; i < 4 * in.GetWidth() * in.GetHeigth(); i += 4)
		{
			if (cw[i + 3] < lower_bound) cw[i + 3] = 0;
			if (cw[i + 3] > upper_bound) cw[i + 3] = 255;
		}
	}


	void fill_graph(double eps)
	{
		if (eps < 0 || eps > 255)
			throw ValueError("the color values should be from 0 to 255!");
		int height = in.GetHeigth();
		int width = in.GetWidth();
		for (int i = 0; i < height; ++i)
		{
			for (int j = 0; j < width; ++j)
			{

				if (j - 1 >= 0 && abs(nod[i * width + j - 1]->input_a -
					nod[i * width + j]->input_a) <= eps)
					union_sets(nod[i * width + j - 1], nod[i * width + j]);

				if (j + 1 < width && abs(nod[i * width + j + 1]->input_a -
					nod[i * width + j]->input_a) <= eps)
					union_sets(nod[i * width + j + 1], nod[i * width + j]);

				if (i - 1 >= 0 && abs(nod[(i - 1) * width + j]->input_a -
					nod[i * width + j]->input_a) <= eps)
					union_sets(nod[(i - 1) * width + j], nod[i * width + j]);

				if (i + 1 < height && abs(nod[(i + 1) * width + j]->input_a -
					nod[i * width + j]->input_a) <= eps)
					union_sets(nod[(i + 1) * width + j], nod[i * width + j]);
			}
		}
	}

	void filter(int sensetive, int R, int G, int B)
	{
		if (R < 0 || G < 0 || B < 0 || R > 255 || G > 255 || B > 255
			|| sensetive < 0 || sensetive > 255)
			throw ValueError("the color values should be from 0 to 255!");
		for (unsigned int i = 0; i < in.GetWidth() * in.GetHeigth(); ++i)
		{
			mst_node* node = find_set(nod[i]);
			if (node->size < sensetive)
			{
				cw[i * 4] = R;
				cw[i * 4 + 1] = G;
				cw[i * 4 + 2] = B;
				cw[i * 4 + 3] = 255;
			}
			else
			{
				cw[i * 4] = 0;
				cw[i * 4 + 1] = 0;
				cw[i * 4 + 2] = 0;
				cw[i * 4 + 3] = 255;
			}
		}
	}

	unsigned int* find_brightest_star()
	{
		unsigned int* coords = new unsigned int[2];
		unsigned int coord = 0;
		int bright = 0;
		unsigned int k = 0;
		for (unsigned int i = 0; i < 4 * in.GetWidth() * in.GetHeigth(); i += 4)
		{
			if (cw[i + 3] > bright)
			{
				bright = cw[i + 3];
				coord = k;
			}
			++k;
		}
		coords[1] = coord / in.GetWidth();
		coords[0] = coord - coords[1] * in.GetWidth();
		this->coords = coords;
		return coords;
	}

	unsigned char* GetPicture() { return this->cw; }

};

class Output {
private:
	unsigned int width;
	unsigned int height;
	const char* path;
	unsigned char* picture;
public:
	Output(unsigned int w, unsigned int h, const char* output_path, const unsigned char* img)
		:width(w), height(h), path(output_path)
	{
		unsigned char* picture;
		size_t size;
		int error = lodepng_encode32(&picture, &size, img, width, height);
		if (!error) {
			lodepng_save_file(picture, size, path);
		}
		if (error)
			throw NameError("Output file error! Please recheck the spelling");
		this->picture = picture;
	}
	~Output()
	{
		delete[] picture;
	}
};

int main() {
	Presenter p;
	std::string filename = "";
	p.set_input_file(filename);
	try
	{
		Input in(filename.c_str());
		ShotHandler sh(in);
		sh.discolor();
		sh.remove_noises(35, 245);
		sh.create_graph();
		sh.fill_graph(20);
		unsigned int* arr = sh.find_brightest_star();
		int* color = p.set_color();
		sh.filter(100, color[0], color[1], color[2]);
		p.set_output_file(filename);
		Output out(in.GetWidth(), in.GetHeigth(), filename.c_str(), sh.GetPicture());
		std::cout << "Brightest star coords: ";
		std::cout << arr[0] << " " << arr[1];
	}
	catch (Error& err)
	{
		std::cout << err.what();
	}
	return 0;
}