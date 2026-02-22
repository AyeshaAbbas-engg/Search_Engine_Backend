#ifndef SORTER_H
#define SORTER_H

#include <vector>
#include <utility>

class Sorter {
public:
    // We use a template so this can sort anything (int, string, or your pairs)
    template <typename T>
    static void quickSort(std::vector<T>& arr, int low, int high) {
        if (low < high) {
            int pivotIndex = partition(arr, low, high);
            quickSort(arr, low, pivotIndex - 1);  // Left side
            quickSort(arr, pivotIndex + 1, high); // Right side
        }
    }

private:
    template <typename T>
    static int partition(std::vector<T>& arr, int low, int high) {
        // We use the last element as the pivot
        T pivot = arr[high]; 
        int i = (low - 1);

        for (int j = low; j < high; j++) {
            // Sort Descending: Higher frequency (second) comes first
            if (arr[j].second > pivot.second) {
                i++;
                std::swap(arr[i], arr[j]);
            }
        }
        std::swap(arr[i + 1], arr[high]);
        return (i + 1);
    }
};

#endif