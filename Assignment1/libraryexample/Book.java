package libraryexample;

public class Book {
    private final String isbn;
    private final String title;
    private final String author;
    private final String shelfLocation;
    private int availableCopies;

    public Book(String isbn, String title, String author,
                String shelfLocation, int availableCopies) {
        if (availableCopies < 0) {
            throw new IllegalArgumentException("availableCopies cannot be negative");
        }
        this.isbn = isbn;
        this.title = title;
        this.author = author;
        this.shelfLocation = shelfLocation;
        this.availableCopies = availableCopies;
    }

    public String getIsbn() {
        return isbn;
    }

    public String getTitle() {
        return title;
    }

    public String getAuthor() {
        return author;
    }

    public String getShelfLocation() {
        return shelfLocation;
    }

    public int getAvailableCopies() {
        return availableCopies;
    }

    public boolean isAvailable() {
        return availableCopies > 0;
    }

    public boolean decreaseAvailableCopy() {
        if (!isAvailable()) {
            return false;
        }
        availableCopies--;
        return true;
    }

    public void increaseAvailableCopy() {
        availableCopies++;
    }

    @Override
    public String toString() {
        return title + " (" + isbn + "), available copies: " + availableCopies;
    }
}
