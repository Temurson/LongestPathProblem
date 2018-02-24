#include <fstream>
#include <iostream>
#include <vector>
#include <queue>
#include <stack>
#include <mutex>

using namespace std;

struct Vertex
{
	vector<int> edges;
	bool used;
	int distance;
};

using Graph = vector<Vertex>;

void readUnweightedUndirectedGraph(Graph& g, istream& in)
{
	int vertex_count, edge_count;
	
	in >> vertex_count >> edge_count;

	g.resize(vertex_count);

	for (int i = 0; i < edge_count; ++i)
	{
		// assume that vertices are counted from 0
		int v, u;
		in >> v >> u;
		g[v].edges.push_back(u);
		g[u].edges.push_back(v);
	}
}

void writeGraphPath(const vector<int>& path, ostream& out)
{
	out << path.size() << endl;
	if (path.size() == 0)
		return;

	bool flag = true;
	for (const int v : path)
	{
		if (flag)
			flag = false;
		else
			out << ' ';
		out << v;
	}
	out << endl;
}

//bool isValidPath(const Graph& graph, const vector<int>& path)
//{
//	if (path.size() < 1)
//		throw runtime_error("path contains 0 vertexes");
//	if (path.size() == 1)
//		return path.at(0) < graph.size();
//
//	for (int i = 0; i < path.size() - 1; ++i)
//	{
//		const vector<int>& edges = graph[path.at(i)].edges;
//		if (find(cbegin(edges), cend(edges), path.at(i + 1)) == cend(edges))
//			return false;
//	}
//
//	return true;
//}

//{ Following functions implement greedy algorithm developed by L.L. Pongrácz

// assign weights to the edges of the given graph.
// weight of the edge is equal to shortest path length
// from root to the corresponding vertex
// returns idx of the farthest vertex from the root
// uses BFS for traversal
int createAuxiliaryGraph(Graph& graph, const int root)
{
	for (int i = 0; i < graph.size(); ++i)
		graph[i].used = false;

	queue<int> q;
	q.push(root);

	graph[root].used = true;
	graph[root].distance = 0;

	int farthestNodeIdx = -1,
		farthestDistance = -1;

	while (!q.empty())
	{
		const int currentVertexIdx = q.front();
		q.pop();

		for (const int next : graph[currentVertexIdx].edges)
		{
			if (!graph[next].used)
			{
				graph[next].used = true;
				q.push(next);
				graph[next].distance = graph[currentVertexIdx].distance + 1;

				if (farthestDistance < graph[next].distance)
				{
					farthestDistance = graph[next].distance;
					farthestNodeIdx = next;
				}
			}
		}
	}

	return farthestNodeIdx;
}

// find long path using greedy algorithm on the constructed
// weighted graph from the previous function
// searches for path starting from root
// returns vertexes making up the path
vector<int> findLongPathFromRoot(Graph& graph, const int root)
{
	for (int i = 0; i < graph.size(); ++i)
		graph[i].used = false;

	stack<int> s;
	s.push(root);

	vector<int> result;

	while (!s.empty())
	{
		const int v = s.top();
		s.pop();
		graph[v].used = true;
		result.push_back(v);

		int farthestNodeIdx = -1,
			farthestNodeDistance = -1;
		for (const int next : graph[v].edges)
		{
			if (!graph[next].used && graph[next].distance > farthestNodeDistance)
			{
				farthestNodeDistance = graph[next].distance;
				farthestNodeIdx = next;
			}
		}

		if (farthestNodeIdx != -1)
			s.push(farthestNodeIdx);
	}

	return result;
}

//}

mutex mtx;

// finds long paths starting from specified range of vertex indexes
// and updates maximum path.
// designed for concurrent execution
// the graph is copied to prevent racing
// longestPath variable is supposed to be shared
void findLongPathInRange(Graph graph,
	const int from, const int to,
	vector<int>& longestPath)
{
	for (int i = from; i < to; ++i)
	{
		const int farthestNode = createAuxiliaryGraph(graph, i);
		
		// i is isolated node
		if (farthestNode == -1)
			continue;

		vector<int> path = move(findLongPathFromRoot(graph, farthestNode));

		//if (!isValidPath(graph, path))
		//	throw runtime_error("invalid path, program contains an error");

		if (longestPath.size() < path.size())
		{
			mtx.lock();
			longestPath = move(path);
			mtx.unlock();
		}
	}
}

int main()
{
	// makes console IO faster
	ios::sync_with_stdio(0); cin.tie(0);

	ifstream in("edges.txt");
	ofstream out("longest_path.txt");

	Graph graph;

	readUnweightedUndirectedGraph(graph, in);

	vector<int> longestPath;

	if (graph.size() < 1000)
		findLongPathInRange(graph, 0, graph.size(), longestPath);
	else
	{
		// make 10 threads
		const int sectionLength = graph.size() / 10;
		vector<thread> threads;

		for (int i = 0, l = 0, r = sectionLength;
			r <= graph.size();
			l += sectionLength, r += sectionLength, ++i)
		{
			if (r + sectionLength > graph.size())
				r = graph.size();
			threads.emplace_back(findLongPathInRange, graph, l, l + sectionLength, ref(longestPath));
		}

		for (auto& t : threads)
			t.join();
	}

	writeGraphPath(longestPath, out);

	in.close();
	out.close();

	return 0;
}
