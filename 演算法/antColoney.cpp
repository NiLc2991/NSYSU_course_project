#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <limits>
#include <random>
#include <algorithm>
#include <string>
#include <sstream>
#include <stdexcept>
#include <ctime>
#include <iomanip>
#include <numeric>

using namespace std;


struct City {
    int id;      // 城市的唯一標識
    double x, y; // 城市的坐標（x, y）
};
int max_x;                  // 最大的x坐標值，用於設置圖像繪製的x軸範圍
int max_y;                  // 最大的y坐標值，用於設置圖像繪製的y軸範圍

string cities_file;         // input檔名
string dataset = "dt1";     // 資料集名稱
int run_times = 30;         // 執行次數
int max_iterations = 30;    // 每Run的迭代次數上限
int num_ants = 100;         // 螞蟻的數量(=population_size)
double alpha = 1.4;         // 費洛蒙的相關係數
double beta_param = 1;      // 距離倒數的相關係數
double rho = 0.3;           // 費洛蒙衰減係數
double Q = 50;              // 常數
int two_opt_runs = 2;       // 設置2-OPT最大優化次數

// 計算兩個城市之間的距離
double calculate_dist(const City &a, const City &b) {

    return sqrt(pow(a.x - b.x, 2) + pow(a.y - b.y, 2));
}

// 從文件中載入城市資料
vector<City> load_cities(const string &filename) {
    ifstream file(filename); 
    if (!file.is_open()) { 
        throw runtime_error("File not found or cannot be opened.");
    }

    vector<City> cities; // 儲存城市資料
    string line; 
    while (getline(file, line)) { 
        istringstream iss(line); 
        City city; // 定義一個城市標的
        if (!(iss >> city.id >> city.x >> city.y)) { 
            // 嘗試從讀取的該行中擷取城市ID、x坐標和y坐標
            throw runtime_error("Invalid file format.");
        }
        if (city.x > max_x) { max_x = city.x; } 
        // 更新最大x坐標值
        if (city.y > max_y) { max_y = city.y; } 
        // 更新最大y坐標值
        cities.push_back(city); 
        // 將城市添加到城市列表中
    }

    return cities; // 返回載入的城市列表
}

// 對給定路徑執行2-OPT優化
vector<int> two_opt(const vector<int> &route, const vector<vector<double>> &dists, int two_opt_runs) {
    // 將初始路徑設置為最佳路徑
    vector<int> bestroute = route;
    // 設置初始最佳距離為無窮大
    double best_distance = numeric_limits<double>::max();
    int run_count = 0; // 優化次數計數器
    // 定義匿名函數計算路徑距離
    auto calculate_route_distance = [&](const vector<int> &r) {
        double distance = 0.0; 
        for (int i = 0; i < r.size() - 1; ++i) {
            distance += dists[r[i]][r[i + 1]]; // 累加相鄰城市的距離
        }
        distance += dists[r.back()][r[0]]; // 將最後一個城市與起始城市的距離加入 (封閉路徑)
        return distance;
    };

    // 計算初始路徑的距離
    best_distance = calculate_route_distance(route);
    bool improved = true; // 標記是否還有改進

    // 不斷嘗試改進路徑，直到無法改進為止或達到最大優化次數
    while (improved && run_count < two_opt_runs) {
        improved = false; // 初始設置為無改進
        // 走訪路徑中的每一對城市索引 (不包括起點與終點)
        for (int i = 1; i < route.size() - 1; ++i) {
            for (int j = i + 1; j < route.size(); ++j) {
                vector<int> new_route = bestroute; // 建立當前最佳路徑的副本
                reverse(new_route.begin() + i, new_route.begin() + j); // 反轉子路徑 [i, j)

                double new_distance = calculate_route_distance(new_route); // 計算反轉後的新距離
                if (new_distance < best_distance) { // 如果新距離優於當前最佳距離
                    best_distance = new_distance;   // 更新最佳距離
                    bestroute = new_route; 
                    improved = true; // 註記路徑有改進
                }
            }
        }
        ++run_count; // 增加優化計數
    }

    return bestroute; // 返回最優化的路徑
}

// 初始化費洛蒙矩陣
vector<vector<double>> init_pheromones(int n, double init_pheromone) {
    // 建立一個 n x n 的二維vector，並將所有元素初始化為初始費洛蒙值
    return vector<vector<double>>(n, vector<double>(n, init_pheromone));
}

// 透過輪盤法進行選擇
int roulette_wheel(const vector<double> &probs) {
    // 計算機率vector的總和
    double sum;
    random_device rd; // 用於亂數生成
    mt19937 gen(rd()); 
    sum = accumulate(probs.begin(), probs.end(), 0.0);
    // 如果總和為 0，表示無法依機率選擇，則隨機返回一個索引

    if (sum == 0.0) {
        uniform_int_distribution<> dis(0, probs.size() - 1);
        return dis(gen); // 使用 std::mt19937
    }

    // 建立範圍在 [0.0, sum] 的均勻分布
    uniform_real_distribution<> dis(0.0, sum);
    double random_num;
    random_num = dis(gen); // 生成亂數

    // 計算累積機率，找到與亂數對應的索引
    double cumulative;
    cumulative = 0.0;
    for (int i = 0; i < probs.size(); ++i) {
        cumulative = cumulative + probs[i]; 
        if (random_num <= cumulative) { // 當亂數小於等於累積值時，選中該索引
            return i;
        }
    }
    return probs.size() - 1;
}

// 執行 (Ant Colony Optimization, ACO) 演算法
pair<double, vector<int>> exec_aco(const vector<City> &allcities) {
    int n;
    n = allcities.size(); // 獲取城市數量
    double init_pheromone = 1;               
    vector<vector<double>> dists(n, vector<double>(n)); // 初始化城市之間的距離矩陣

    // 計算每對城市之間的距離，並填入距離矩陣
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            dists[i][j] = calculate_dist(allcities[i], allcities[j]); 
        }
    }

    // 初始化費洛蒙矩陣，所有費洛蒙值設為初始值
    vector<vector<double>> pheromones = init_pheromones(n, init_pheromone);
    vector<int> best_route; // 儲存目前找到的最佳路徑
    double best_dist = numeric_limits<double>::max(); // 將最佳距離初始化為無窮大

    // 迭代多次以模擬蟻群搜索過程
    for (int iteration = 0; iteration < max_iterations; ++iteration) {
        vector<vector<int>> antroutes(num_ants); // 儲存每隻螞蟻的路徑
        vector<double> ant_dists(num_ants, 0.0); // 儲存每隻螞蟻的路徑總距離

        // 每隻螞蟻依序構建自己的路徑
        for (int ant = 0; ant < num_ants; ++ant) {
            vector<int> visited(n, 0); // 記錄城市是否已被訪問 (0 表示未訪問)
            vector<int> route; // 儲存當前螞蟻的路徑
            route.push_back(0); // 從第 0 號城市開始
            visited[0] = 1; // 標記第 0 號城市已訪問

            // 螞蟻依次訪問剩餘城市，直到完成路徑
            for (int step = 1; step < n; ++step) {
                int currentcity;
                currentcity = route.back(); // 當前所在的城市
                vector<double> probs(n, 0.0); // 儲存每個目標城市的選擇機率

                // 計算當前城市到其他未訪問城市的機率
                for (int nextcity = 0; nextcity < n; ++nextcity) {
                    if (!visited[nextcity]) { // 只考慮未訪問的城市
                        probs[nextcity] = pow(pheromones[currentcity][nextcity], alpha) *
                                                   pow(1.0 / dists[currentcity][nextcity], beta_param); // 費洛蒙強度與距離的權重組合
                    }
                }

                // 使用輪盤法決定下一個城市
                int nextcity = roulette_wheel(probs);
                route.push_back(nextcity); // 將選定的城市加入路徑
                visited[nextcity] = 1; // 標記選定的城市為已訪問
            }

            route.push_back(0);                 // 回到起始城市，完成封閉
            //antroutes[ant] = two_opt(antroutes[ant], dists, max_two_opt_iterations);
            //route = two_opt(route, dists);      // 對路徑進行 2-OPT 優化 
            route = two_opt(route, dists, two_opt_runs);   // 呼叫 2_OPT 並限制優化次數 
            antroutes[ant] = route;             // 儲存該螞蟻的完整路徑

            // 計算該路徑的總距離
            for (int k = 0; k < route.size() - 1; ++k) {
                ant_dists[ant] = ant_dists[ant] + dists[route[k]][route[k + 1]]; // 累加路徑中每段距離
            }

            // 如果該螞蟻的路徑距離小於當前最佳距離，更新最佳路徑與距離
            if (ant_dists[ant] < best_dist) {
                best_dist = ant_dists[ant];
                best_route = route;
            }
        }

        // 更新費洛蒙矩陣
        for (int i = 0; i < n; ++i) {
            for (int j = 0; j < n; ++j) {
                pheromones[i][j] = pheromones[i][j] * (1 - rho); // 所有費洛蒙值衰減 (根據衰減係數 rho)
            }
        }

        // 根據每隻螞蟻的路徑距離，添加新的費洛蒙
        for (int ant = 0; ant < num_ants; ++ant) {
            double benefit;
            benefit = Q / ant_dists[ant]; // 計算該路徑的費洛蒙貢獻值
            for (int k = 0; k < antroutes[ant].size() - 1; ++k) {
                int now = antroutes[ant][k]; // 當前城市
                int next = antroutes[ant][k + 1]; // 下一個城市
                pheromones[now][next] = pheromones[now][next] + benefit; // 增加費洛蒙值
                pheromones[next][now] = pheromones[next][now] + benefit; // 由於路徑是對稱的，因此反向更新
            }
        }
    }

    // 返回最佳距離與最佳路徑
    return {best_dist, best_route};
}



// 將計算結果保存到文件中
void save_results(const string &filename, double mean_distance, double best_distance, const vector<int> &best_route, const vector<City> &cities) {
    ofstream file(filename); // 打開指定的文件以進行寫入操作
    file << "mean distance: " << fixed << setprecision(3) << mean_distance << "\n"; 
    // 輸出平均距離(mean_distance)，使用固定小數點格式，保留三位小數
    file << "distance: " << fixed << setprecision(3) << best_distance << "\n"; 
    // 輸出最佳路徑距離(best_distance)，格式同上
    int k=0;
    for (int city : best_route) {
        if (k == 0 || cities[city].id != 1) {
            file << cities[city].id << "\n"; 
            // 逐行輸出最佳路徑上的城市編號（使用城市編號而非索引）
        }
        k++;
    }
}


// 使用Gnuplot繪製路徑圖
void plot_route(const string &filename, const vector<City> &cities, const vector<int> &route) {
    ofstream data("route.txt"); // 打開"route.txt"以保存路徑數據
    if (!data) {
        throw runtime_error("Failed to open route.txt for writing."); 
    }
    for (int city : route) {
        data << cities[city].x << " " << cities[city].y << "\n";         // 將每個城市的坐標 (x, y) 寫入文件，每行對應一個城市
    }
    data << cities[route[0]].x << " " << cities[route[0]].y << "\n";     // 將起點城市的坐標重新寫入，以完成封閉
    data.close(); // 關閉文件

    ofstream gnuplot("plot.gp"); // 創造或打開"plot.gp"以生成Gnuplot script
    if (!gnuplot) {
        throw runtime_error("Failed to open plot.gp for writing."); 
    }
    // 生成Gnuplot script的內容
    gnuplot << "set terminal png size 800,800\n"; // 指定輸出為800x800像素的PNG圖片
    gnuplot << "unset key\n"; 
    if (max_x > 0 && max_x < 100) {
        gnuplot << "set xrange [0:100]\n"; // x座標若小於100則x軸範圍設為[0, 100]
    }
    if (max_y > 0 && max_y < 100) {
        gnuplot << "set yrange [0:100]\n"; // y座標若小於100則y軸範圍設為[0, 100]
    }
    gnuplot << "set output '" << filename << "'\n";                                     // 指定輸出的圖片文件名
    gnuplot << "set title '" << dataset << "'\n";                                       // 設置圖片的標題為擷取出的路徑圖名稱
    gnuplot << "plot 'route.txt' with linespoints pointtype 7 linecolor 'blue'\n";      // 繪製路徑數據
    gnuplot.close(); // 關閉Gnuplot script

    int result = system("gnuplot plot.gp"); 
    // 呼叫系統命令執行Gnuplot script，生成路徑圖
    if (result != 0) {
        throw runtime_error("Failed to execute Gnuplot."); 

    }
}

// 提示使用者輸入參數
void inputParameters(string& dataset, int& run_times, int& max_iterations, 
                     int& num_ants, double& alpha, double& beta_param, 
                     double& rho, double& Q, int& two_opt_runs) {
    cout << "Enter dataset (default: " << dataset << "): ";
    string input;
    getline(cin, input);
    if (!input.empty()) dataset = input;

    cout << "Enter run times (default: " << run_times << "): ";
    getline(cin, input);
    if (!input.empty()) run_times = stoi(input);

    cout << "Enter max iterations (default: " << max_iterations << "): ";
    getline(cin, input);
    if (!input.empty()) max_iterations = stoi(input);

    cout << "Enter number of ants (default: " << num_ants << "): ";
    getline(cin, input);
    if (!input.empty()) num_ants = stoi(input);

    cout << "Enter alpha (default: " << alpha << "): ";
    getline(cin, input);
    if (!input.empty()) alpha = stod(input);

    cout << "Enter beta (default: " << beta_param << "): ";
    getline(cin, input);
    if (!input.empty()) beta_param = stod(input);

    cout << "Enter rho (default: " << rho << "): ";
    getline(cin, input);
    if (!input.empty()) rho = stod(input);

    cout << "Enter Q (default: " << Q << "): ";
    getline(cin, input);
    if (!input.empty()) Q = stod(input);

    cout << "Enter max runs of 2-OPT (default: " << two_opt_runs << "): ";
    getline(cin, input);
    if (!input.empty()) two_opt_runs = stoi(input);
}

int main(int argc, char *argv[]) {
    // 亦可以命令列模式執行:  .\hw4.exe <input_name> <dataset> <run_times> <iteration> <num_ants> <alpha> <beta> <rho> <Q> <two_opt_runs>
    //char dataset[];             // 資料集名稱

    if (argc > 1) {
        cities_file = argv[1]; 
        dataset = argv[2];
        run_times = atoi(argv[3]);
        max_iterations = atoi(argv[4]);
        num_ants = atoi(argv[5]);
        alpha = atof(argv[6]);
        beta_param = atof(argv[7]);
        rho = atof(argv[8]);
        Q = atof(argv[9]);
        two_opt_runs = atoi(argv[10]);
        // 如果命令行參數中提供了文件檔名，直接使用該文件名
    } else {
        cout << "Enter the filename (or type exit to quit): "; 
        // 提示用戶輸入城市資料文件名稱或退出
        while (getline(cin, cities_file)) { 
            // 讀取用戶輸入的文件名
            if (cities_file == "exit") { 
            //if (cities_file.empty() || cities_file == "exit") { 
                // 如果輸入為空或用戶選擇退出，程序結束
                return 0;
            }

            ifstream file(cities_file); 
            // 嘗試打開用戶輸入的文件
            if (file.is_open()) {
                break; 
                // 如果文件打開成功，退出並使用該文件
            } else {
                cout << "File not found. Please try again (or type exit to quit): "; 
                // 如果文件不存在，提示用戶重新輸入文件名
            }
        }
        inputParameters(dataset, run_times, max_iterations, num_ants, alpha, beta_param, rho, Q, two_opt_runs);
 
    }
    // 印出所有參數
    cout << "\nParameters:\n";
    //cout << "Input file: " << cities_file << "\n";
    cout << "Dataset: " << dataset << "\n";
    cout << "Run times: " << run_times << "\n";
    cout << "Max iterations: " << max_iterations << "\n";
    cout << "Number of ants: " << num_ants << "\n";
    cout << "Alpha: " << alpha << "\n";
    cout << "Beta: " << beta_param << "\n";
    cout << "Rho: " << rho << "\n";
    cout << "Q: " << Q << "\n";
    cout << "Max runs of 2-OPT: " << two_opt_runs << "\n";
    try {
        vector<City> cities = load_cities(cities_file); 
        // 載入城市資料，返回一個城市的vector 
        cout << "Enter output text filename: "; 
        string text_file;
        cin >> text_file;

        cout << "Enter output image filename: "; 

        string img_file;
        cin >> img_file;

        double total_distance = 0.0; 
        // 累積總距離，用於計算平均距離
        vector<int> best_route; 
        // 用於儲存當前最佳路徑的城市序列
        double best_distance = numeric_limits<double>::max(); 
        // 初始化最佳距離為一個極大的值

        for (int i = 0; i < run_times; ++i) { 
            auto [distance, route] = exec_aco(cities); 
            // 用ACO算法計算一輪結果，返回路徑距離和城市序列
            printf("(%d) distance: %f\n",i+1, distance);  
            total_distance += distance; 
            // 累加本輪的路徑距離

            if (distance < best_distance) { 
                // 如果本輪結果優於當前最佳結果，更新最佳結果
                best_distance = distance;
                best_route = route;
            }                


        }

        double mean_distance = total_distance / run_times; 
        // 計算30次運行的平均距離
        save_results(text_file, mean_distance, best_distance, best_route, cities); 
        // 將結果保存到文本文件中
        plot_route(img_file, cities, best_route); 
        // 使用Gnuplot繪製最佳路徑圖並保存到圖檔

        // 印出結果
        cout << "Mean distance: " << mean_distance << endl; 
        cout << "Best distance: " << best_distance << endl;
        cout << "Results saved to " << text_file << " and " << img_file << endl;
    } catch (const exception &e) { 

        cerr << "Error: " << e.what() << endl;
        return 1; 
        // 返回1表示程序異常退出
    }

    return 0; 
    // 返回0表示程序正常執行完成
}

