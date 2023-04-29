#include<iostream>
#include<string>
#include<fstream>
#include<vector>
#include<cmath>
using namespace std;
ifstream web_graph_file("Webgraph.csv");
ifstream impressions_file("NumberOfImpressions.csv");
ifstream keywords_file("Keywords.csv");


class Edge
{
public:
	string src, dest;
};

class Graph
{
public:

	// V-> Number of vertices, E-> Number of edges
	int V, E;

	Edge* edges;
	vector<string> vertices;
	float* pagerank_norm;
	float* score;
	int* impressions;
	int* clicks;
	float* CTR;
	vector<vector<string>> keywords;
};

void get_edges(Graph &graph)   // get edges and vertices
{
	int E = -1; string x;
	vector<string>w;

	do
	{
		web_graph_file >> x;
		w.push_back(x);
		E++;
	} while (!web_graph_file.eof());

	graph.E = E;
	graph.edges = new Edge[E];
	for (size_t i = 0; i < E; i++)
	{
		x = w[i].substr(w[i].find(".") + 1, w[i].find(",") - 9);
		w[i].erase(0, w[i].find(","));
		graph.edges[i].src = x;

		x = w[i].substr(w[i].find(".") + 1);
		x.erase(x.find("."));
		graph.edges[i].dest = x;
	}

	// getting the vertices
	vector<string> temp;
	for (size_t i = 0; i < E; i++)
	{
		temp.push_back(graph.edges[i].src);
		temp.push_back(graph.edges[i].dest);
	}

	graph.vertices.push_back(temp[0]);

	for (size_t i = 0, j = 0; i < 2 * E; i++, j = 0)
	{
		for (; j < graph.vertices.size();)
		{
			if (temp[i] == graph.vertices[j])
				break;
			j++;
		}
		if (j == graph.vertices.size())
			j--;
		if (graph.vertices[j] != temp[i])
			graph.vertices.push_back(temp[i]);
	}
	graph.V = graph.vertices.size();
	web_graph_file.close();
	 
}

void get_impressions(Graph &graph)
{
	string x;
	graph.impressions = new int[graph.V];
	graph.clicks = new int[graph.V];
	graph.CTR = new float[graph.V];
	for (size_t i = 0; i < graph.V; i++)
	{
		impressions_file >> x;
		if (x.size() == 1 && x[0] == 34)  // x = """
			impressions_file >> x;

		x.erase(0, x.find(",") + 1);
		graph.clicks[i] = stoi(x.substr(x.find(",") + 1));
		x.erase(x.find(","));
		graph.impressions[i] = stoi(x);
	}
	//how to get the CTR: clicks ÷ impressions (%)
	for (size_t i = 0; i < graph.V; i++)
	{
		graph.CTR[i] = float(graph.clicks[i]) / float(graph.impressions[i]);
	}
	impressions_file.close();
}

void get_keywords(Graph &graph)
{
	string x;
	graph.keywords.resize(graph.V);

	for (size_t i = 0; i < graph.V; i++)
	{
		getline(keywords_file, x);
		if (x.size() == 1 && x[0] == 34)  // x = """
			keywords_file >> x;

		x = x.substr(x.find(",") + 1);

		while (x.size() > 0)
		{
			if (x.find(",") < 1000)
			{
				graph.keywords[i].push_back(x.substr(0, x.find(",")));
				x.erase(0, x.find(",") + 1);
			}
			else     // last keyword
			{
				graph.keywords[i].push_back(x.substr(0, x.size() - 2));
				break;
			}
		}
	}
	keywords_file.close();
}

void calc_PR(Graph &graph)
{
	int V = graph.V;
	graph.pagerank_norm = new float[V];
	float* temp1 = new float[V], *temp2 = new float[V];

	for (size_t i = 0; i < V; i++)
		temp1[i] = 1.0 / V;


	//to get PR
	for (int x = 0; x < 50; x++)
	{
		for (size_t i = 0; i < V; i++)
		{
			temp2[i] = 0;
			for (size_t j = 0; j < graph.E; j++)
			{
				float numerator = 0, denominator = 0;
				if (graph.edges[j].dest == graph.vertices[i])
				{
					int l = 0;
					for (; l < V; l++)
					{
						if (graph.edges[j].src == graph.vertices[l])   // to get the location of the vertix pointing to the one we are calculating PR for and so its location in temp1 too
							break;
					}
					numerator += temp1[l];
					for (size_t k = 0; k < graph.E; k++)
						if (graph.edges[k].src == graph.edges[j].src)
							denominator++;
				}
				if (denominator != 0)
					temp2[i] += (numerator / denominator);
			}
		}

		
		for (size_t i = 0; i < V; i++)
			temp1[i] = temp2[i];
	}

	//to get PR normalized
	float max = temp1[0], min = temp1[0];
	for (size_t i = 0; i < V; i++)
	{
		if (temp1[i] > max)
			max = temp1[i];
		if (temp1[i] < min)
			min = temp1[i];
	}
	for (size_t i = 0; i < V; i++)
		graph.pagerank_norm[i] = (temp1[i] - min) / (max - min);
	
	delete[] temp1;
	delete[] temp2;
}

void calc_score(Graph &graph)
{
	graph.score = new float[graph.V];
	for (int i = 0; i < graph.V; i++)
	{
		graph.score[i] = 0.4 * graph.pagerank_norm[i] + ((1 - ((0.1 * graph.impressions[i]) / (1 + 0.1 * graph.impressions[i]))
			* graph.pagerank_norm[i] + ((0.1 * graph.impressions[i]) / (1 + 0.1 * graph.impressions[i])) * graph.CTR[i])) * 0.6;
	}
}

vector<string> search_results(string x, Graph graph)
{
	vector<string> results; vector<int> num_of_site;
	vector<string> ranked_results;
	if (x[0] == 34 && x[x.size() - 1] == 34)
	{
		x = x.substr(1, x.size() - 2);
		for (size_t i = 0; i < graph.V; i++)
			for (size_t j = 0; j < graph.keywords[i].size(); j++)
				if (x == graph.keywords[i][j])
				{
					results.push_back(graph.vertices[i]);
					num_of_site.push_back(i);
					break;
				}
		for (size_t i = 0; i < results.size(); )
		{
			float min = 99999; string y; int k = 0;
			for (size_t j = 0; j < results.size(); j++)
			{
				if (graph.score[num_of_site[j]] < min)
				{
					min = graph.score[num_of_site[j]];
					y = graph.vertices[num_of_site[j]];
					k++;
				}
			}
			ranked_results.push_back(y);
			results.erase(results.begin() + k - 1);
			num_of_site.erase(num_of_site.begin() + k - 1);
		}
		return ranked_results;
	}
	if (x.substr(x.find(" ") + 1, 3) == "AND")
	{
		vector<string> results2; vector<int> num_of_site2; vector<string> results3; vector<int> num_of_site3;
		string y = x.substr(x.find(" ") + 5);
		x.erase(x.find(" "));
		for (size_t i = 0; i < graph.V; i++)
			for (size_t j = 0; j < graph.keywords[i].size(); j++)
			{
				if (graph.keywords[i][j].find(" ") < 1000) // if the keword is two words
				{
					if (x == graph.keywords[i][j].substr(0, graph.keywords[i][j].find(" "))
						|| x == graph.keywords[i][j].substr(graph.keywords[i][j].find(" ") + 1))
					{
						results2.push_back(graph.vertices[i]);
						num_of_site2.push_back(i);
						break;
					}
				}
				else if (x == graph.keywords[i][j])
				{
					results2.push_back(graph.vertices[i]);
					num_of_site2.push_back(i);
					break;
				}
			}
		for (size_t i = 0; i < graph.V; i++)
			for (size_t j = 0; j < graph.keywords[i].size(); j++)
			{
				if (graph.keywords[i][j].find(" ") < 1000) // if the keword is two words
				{
					if (y == graph.keywords[i][j].substr(0, graph.keywords[i][j].find(" "))
						|| y == graph.keywords[i][j].substr(graph.keywords[i][j].find(" ") + 1))
					{
						results3.push_back(graph.vertices[i]);
						num_of_site3.push_back(i);
						break;
					}
				}
				else if (y == graph.keywords[i][j])
				{
					results3.push_back(graph.vertices[i]);
					num_of_site3.push_back(i);
					break;
				}
			}

		for (size_t i = 0; i < results2.size(); i++)
		{
			for (size_t j = 0; j < results3.size(); j++)
			{
				if (results2[i] == results3[j])
				{
					results.push_back(results3[j]);
					num_of_site.push_back(num_of_site3[j]);
					break;
				}
			}
		}

		for (size_t i = 0; i < results.size(); )
		{
			float min = 99999; string y; int k = 0;
			for (size_t j = 0; j < results.size(); j++)
			{
				if (graph.score[num_of_site[j]] < min)
				{
					min = graph.score[num_of_site[j]];
					y = graph.vertices[num_of_site[j]];
					k++;
				}
			}
			ranked_results.push_back(y);
			results.erase(results.begin() + k - 1);
			num_of_site.erase(num_of_site.begin() + k - 1);
		}
		return ranked_results;
	}
	if (x.substr(x.find(" ") + 1, 2) == "OR")
	{
		string y = x.substr(x.find(" ") + 4);
		x.erase(x.find(" "));
		for (size_t i = 0; i < graph.V; i++)
			for (size_t j = 0; j < graph.keywords[i].size(); j++)
			{
				if (graph.keywords[i][j].find(" ") < 1000) // if the keword is two words
				{
					if (x == graph.keywords[i][j].substr(0, graph.keywords[i][j].find(" "))
						|| x == graph.keywords[i][j].substr(graph.keywords[i][j].find(" ") + 1)
						|| y == graph.keywords[i][j].substr(0, graph.keywords[i][j].find(" "))
						|| y == graph.keywords[i][j].substr(graph.keywords[i][j].find(" ") + 1))
					{
						results.push_back(graph.vertices[i]);
						num_of_site.push_back(i);
						break;
					}
				}
				else if (x == graph.keywords[i][j] || y == graph.keywords[i][j])
				{
					results.push_back(graph.vertices[i]);
					num_of_site.push_back(i);
					break;
				}
			}

		for (size_t i = 0; i < results.size(); )
		{
			float min = 99999; string y; int k = 0;
			for (size_t j = 0; j < results.size(); j++)
			{
				if (graph.score[num_of_site[j]] < min)
				{
					min = graph.score[num_of_site[j]];
					y = graph.vertices[num_of_site[j]];
					k++;
				}
			}
			ranked_results.push_back(y);
			results.erase(results.begin() + k - 1);
			num_of_site.erase(num_of_site.begin() + k - 1);
		}
		return ranked_results;
	}
	else        //plain search
	{
		if (x.find(" ") < 1000) //plain search with two search words
		{
			x = x.insert(x.find(" ") + 1, "OR ");
			return search_results(x, graph);
		}

		else //else it is plain search with one search word
		{
			for (size_t i = 0; i < graph.V; i++)
				for (size_t j = 0; j < graph.keywords[i].size(); j++)
				{
					if (graph.keywords[i][j].find(" ") < 1000) // if the keword is two words
					{
						if (x == graph.keywords[i][j].substr(0, graph.keywords[i][j].find(" "))
							|| x == graph.keywords[i][j].substr(graph.keywords[i][j].find(" ") + 1))
						{
							results.push_back(graph.vertices[i]);
							num_of_site.push_back(i);
							break;
						}
					}
					else if (x == graph.keywords[i][j])
					{
						results.push_back(graph.vertices[i]);
						num_of_site.push_back(i);
						break;
					}
				}
			for (size_t i = 0; i < results.size(); )
			{
				float min = 99999; string y; int k = 0;
				for (size_t j = 0; j < results.size(); j++)
				{
					if (graph.score[num_of_site[j]] < min)
					{
						min = graph.score[num_of_site[j]];
						y = graph.vertices[num_of_site[j]];
						k++;
					}
				}
				ranked_results.push_back(y);
				results.erase(results.begin() + k - 1);
				num_of_site.erase(num_of_site.begin() + k - 1);
			}
			return ranked_results;
		}
	}
}

void update_impressions(vector<string> results, Graph& graph)
{
	vector<string> x;
	x.resize(graph.V);
	impressions_file.open("NumberOfImpressions.csv");
	for (size_t i = 0; i < graph.V; i++)
	{
		impressions_file >> x[i];
		if (x[i].size() == 1 && x[i][0] == 34)  // x = """
			impressions_file >> x[i];
	}

	for (size_t i = 0; i < results.size(); i++)
	{
		for (size_t j = 0; j < graph.V; j++)
		{
			if (results[i] == x[j].substr(x[j].find(".") + 1, x[j].find(",") - 9))
			{
				graph.impressions[j] += 1;
				break;
			}
		}
	}
	//updating the CTR and the score
	for (size_t i = 0; i < graph.V; i++)
	{
		graph.CTR[i] = float(graph.clicks[i]) / float(graph.impressions[i]);
	}
	calc_score(graph);
	impressions_file.close();
}

void update_clicks(string clicked, Graph& graph)
{
	vector<string> x;
	x.resize(graph.V);
	impressions_file.open("NumberOfImpressions.csv");
	for (size_t i = 0; i < graph.V; i++)
	{
		impressions_file >> x[i];
		if (x[i].size() == 1 && x[i][0] == 34)  // x = """
			impressions_file >> x[i];
	}

	for (size_t j = 0; j < graph.V; j++)
	{
		if (clicked == x[j].substr(x[j].find(".") + 1, x[j].find(",") - 9))
		{
			graph.clicks[j] += 1;
			break;
		}
	}
	//updating the CTR and the score
	for (size_t i = 0; i < graph.V; i++)
	{
		graph.CTR[i] = float(graph.clicks[i]) / float(graph.impressions[i]);
	}
	calc_score(graph);
	impressions_file.close();
}

void search(Graph& graph)
{
	cout << "\nWhat would you like to search for?    *try searching for \"data\"   \nSearch for: ";
	string x;
	cin.ignore();
	getline(cin, x);
	vector<string> results;
	results = search_results(x, graph);
	cout << "\n\nSearch results:\n";
	for (size_t i = 0; i < results.size(); i++)
		cout << i + 1 << ". " << results[i] << endl;
	update_impressions(results, graph);
	cout << "\n Would you like to:   \n1. Choose a webpage to open  \n2. New search  \n3. Exit  \n";
	cout << "Type in the number of your choice: ";
	int choice2;
	cin >> choice2;
	while (choice2 != 1 && choice2 != 2 && choice2 != 3)
	{
		cout << "\nPlease enter a valid number: ";
		cin >> choice2;
	}
	if (choice2 == 3)
	{
		cout << "\nGood bye";
		return;
	}
	if (choice2 == 2)
	{
		search(graph);
		return;
	}
	if (choice2 == 1)
	{
		cout << "Please type in the number of the webpage you would like to open: ";
		cin >> choice2;
		update_clicks(results[choice2 - 1], graph);
		cout << "\nYou're now viewing www." << results[choice2 - 1] << ".com.\n";
		cout << "Would you like to:   \n1. Go back to search results  \n2. New search  \n3. Exit\n ";
		cout << "Type in the number of your choice: ";
		cin >> choice2;
		while (choice2 != 1 && choice2 != 2 && choice2 != 3)
		{
			cout << "\nPlease enter a valid number: ";
			cin >> choice2;
		}
		if (choice2 == 3)
		{
			cout << "\nGood bye";
			return;
		}
		if (choice2 == 2)
		{
			search(graph);
			return;
		}
		if (choice2 == 1)
		{
			for (size_t i = 0; i < results.size(); i++)
				cout << i + 1 << ". " << results[i] << endl;
			update_impressions(results, graph);
			cout << "Please type in the number of the webpage you would like to open: ";
			cin >> choice2;
			update_clicks(results[choice2 - 1], graph);
			cout << "\nYou're now viewing www." << results[choice2 - 1] << ".com.\n";
			cout << "Would you like to:  \n1. New search  \n2. Exit\n ";
			while (choice2 != 1 && choice2 != 2)
			{
				cout << "\nPlease enter a valid number: ";
				cin >> choice2;
			}
			if (choice2 == 2)
			{
				cout << "\nGood bye";
				return;
			}
			if (choice2 == 1)
			{
				search(graph);
				return;
			}
		}
	}
}



void main()
{
	Graph web_graph;
	get_edges(web_graph);
	get_impressions(web_graph);
	get_keywords(web_graph);
	calc_PR(web_graph);
	calc_score(web_graph);
	int choice;
	cout << "Welcome!   \nWhat would you like to do?   \n1. New search   \n2. Exit   \n\n";
	cout << "Type in the number of your choice: ";
	cin >> choice;
	while (choice != 1 && choice != 2)
	{
		cout << "Please enter a valid number: ";
		cin >> choice;
	}
	if (choice == 1)
	{
		search(web_graph);
	}
	if (choice == 2)
	{
		cout << "Good bye";
		return;
	}
}